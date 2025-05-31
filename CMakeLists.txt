cmake_minimum_required(VERSION 3.2.1)
project(pos_projekt)

set(CMAKE_CXX_STANDARD 20)

message(STATUS "Zmienna środowiskowa MOJA_ZMIENNA: $ENV{OPENCV_DIR}\\x64\\vc16\\lib")

include_directories("$ENV{OPENCV_DIR}\\include" "include")
link_directories("$ENV{OPENCV_DIR}\\x64\\vc16\\lib")

add_executable(pos_projekt main.cpp src/ini.c)

target_link_libraries(pos_projekt "opencv_world4110d")

