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


    // Wczytanie configu

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Nie można otworzyć pliku: " << argv[1] << std::endl;
        return 1;
    }

    std::string config[2];
    int i = 0;

    while (std::getline(file, config[i])) {
        i += 1;
        if (i == 2) break;
    }

    std::cout << config[0] << "\n" << config[1] << "\n";
 
    config[0] = config[0].substr(config[0].find(" ") + 1);
    config[1] = config[1].substr(config[1].find(" ") + 1);


    // Cos nowego



    return 0;
}