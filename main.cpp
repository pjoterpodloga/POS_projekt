#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>

namespace fs = std::filesystem;

/// Global mutex for synchronizing console output
std::mutex cout_mutex;
/// Path to input directory read from INI
std::string input_dir;
/// Path to output directory read from INI
std::string output_dir;
/// Atomic counter of processed images
std::atomic<int> processed_count(0);

/// Typ wskaźnika do funkcji obsługi wpisów INI
typedef int (*ini_handler)(void* user, const char* section, const char* name, const char* value);

/**
 * @brief Usuwa białe znaki z końca napisu.
 * @param s Wejściowy napis.
 * @return Wskaźnik na początek napisu bez białych znaków na końcu.
 */
static char* rstrip(char* s) {
    char* p = s + strlen(s);
    while (p > s && (unsigned char)(*--p) <= ' ') *p = '\0';
    return s;
}

/**
 * @brief Pomija początkowe białe znaki w napisie.
 * @param s Wejściowy napis.
 * @return Wskaźnik na pierwszy nie-biały znak w napisie.
 */
static char* lskip(const char* s) {
    while (*s && (unsigned char)(*s) <= ' ') s++;
    return (char*)s;
}

/**
 * @brief Parsuje plik INI.
 * @param filename Ścieżka do pliku INI.
 * @param handler Funkcja wywoływana dla każdej pary klucz-wartość.
 * @param user Wskaźnik użytkownika przekazywany do handlera.
 * @return 0 jeśli sukces, numer linii z błędem w przeciwnym wypadku, -1 jeśli nie można otworzyć pliku.
 */
int ini_parse(const char* filename, ini_handler handler, void* user) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;

    char line[200];
    char section[50] = "";
    char prev_name[50] = "";
    int lineno = 0;
    int error = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        lineno++;
        char* start = lskip(line);
        if (*start == ';' || *start == '#') continue;
        if (*start == '[') {
            char* end = strchr(start + 1, ']');
            if (!end) { error = lineno; break; }
            *end = '\0';
            strncpy(section, start + 1, sizeof(section) - 1);
            section[sizeof(section) - 1] = '\0';
            *prev_name = '\0';
        } else if (*start) {
            char* sep = strchr(start, '=');
            if (!sep) { error = lineno; break; }
            *sep = '\0';
            char* name = rstrip(start);
            char* value = lskip(sep + 1);
            rstrip(value);
            strncpy(prev_name, name, sizeof(prev_name) - 1);
            prev_name[sizeof(prev_name) - 1] = '\0';
            if (!handler(user, section, name, value)) { error = lineno; break; }
        }
    }
    fclose(file);
    return error;
}

/**
 * @brief Handler dla wpisów INI sekcji [Paths].
 * @param user Wskaźnik użytkownika (nieużywany).
 * @param section Nazwa sekcji.
 * @param name Nazwa klucza.
 * @param value Wartość klucza.
 * @return Zawsze 1, aby kontynuować parsowanie.
 */
int my_ini_handler(void* user, const char* section, const char* name, const char* value) {
    if (std::string(section) == "Paths") {
        if (std::string(name) == "input_dir") input_dir = value;
        else if (std::string(name) == "output_dir") output_dir = value;
    }
    return 1;
}

/**
 * @brief Wykrywa krawędzie w obrazie i zwraca obraz krawędzi.
 * @param image Wejściowy obraz kolorowy.
 * @return Obraz krawędzi w formacie BGR.
 */
cv::Mat detect_edges(const cv::Mat& image) {
    cv::Mat gray, edges;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Canny(gray, edges, 100, 200);
    cv::cvtColor(edges, edges, cv::COLOR_GRAY2BGR);
    return edges;
}

/**
 * @brief Przetwarza pojedynczy obraz: wykrywa krawędzie, zapisuje wynik i tworzy miniaturki.
 * @param path Ścieżka do pliku obrazu.
 * @param thumbs_original Wektor miniatur oryginalnych obrazów.
 * @param thumbs_processed Wektor miniatur przetworzonych obrazów.
 * @param thumb_size Rozmiar miniaturki (kwadrat).
 */
void process_image(const fs::path& path, std::vector<cv::Mat>& thumbs_original, std::vector<cv::Mat>& thumbs_processed, int thumb_size = 100) {
    try {
        cv::Mat img = cv::imread(path.string());
        if (img.empty()) return;
        cv::Mat edges = detect_edges(img);
        std::string out_path = output_dir + "/" + path.filename().string();
        cv::imwrite(out_path, edges);

        {
            std::lock_guard<std::mutex> lock(output_mutex);
            thumbs_original.push_back(th_o);
            thumbs_processed.push_back(th_p);
        }
        processed_count++;
    } catch (...) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Błąd przetwarzania pliku: " << path << "\n";
    }
}

/**
 * @brief Główna funkcja programu.
 * @param argc Liczba argumentów linii poleceń.
 * @param argv Tablica argumentów (pierwszy to ścieżka do INI).
 * @return Kod zakończenia (0 = sukces, 1 = błąd).
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uzycie: " << argv[0] << " config.ini\n";
        return 1;
    }
    if (ini_parse(argv[1], my_ini_handler, nullptr) < 0) {
        std::cerr << "Nie mozna zaladować pliku INI\n";
        return 1;
    }
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        std::cerr << "Nieprawidlowa sciezka wejsciowa: " << input_dir << "\n";
        return 1;
    }
    fs::create_directories(output_dir);

    std::vector<fs::path> image_files;
    for (auto& entry : fs::directory_iterator(input_dir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".jpg" || ext == ".png" || ext == ".bmp")
            image_files.push_back(entry.path());
    }
    
    

    std::vector<cv::Mat> thumbs_orig, thumbs_proc;
    std::vector<std::thread> threads;
    unsigned int max_t = std::thread::hardware_concurrency();
    unsigned int limit = max_t ? max_t : 4;
    for (auto& path : image_files) {
        threads.emplace_back([&path, &thumbs_orig, &thumbs_proc]() {
            process_image(path, thumbs_orig, thumbs_proc);
        });
        if (threads.size() >= limit) {
            for (auto& t : threads) t.join();
            threads.clear();
        }
    }

    // TODO: stworzenie kolarzu
    std::cout << "Przetworzono " << processed_count.load() << " obrazów.\n";
    return 0;
}
