cmake_minimum_required(VERSION 3.29.5)
project(pos_projekt)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(pos_projekt main.cpp)

