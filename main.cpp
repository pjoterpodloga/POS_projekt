#include <iostream>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <string>

int main(int argc, char** argv){

    std::cout << "Hello and welcome to " << CV_VERSION << "!\n";

    if (argc < 2) {
        std::cerr << "Użyj: " << argv[0] << " config.ini" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Nie można otworzyć pliku: " << argv[1] << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    return 0;
}