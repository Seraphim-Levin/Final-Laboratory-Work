cmake_minimum_required(VERSION 4.0)
project(generate_2_0)

set(CMAKE_CXX_STANDARD 20)

add_executable(generate_2_0 src/Sales.cpp
        src/generate_data_mf.cpp
        include/generate_data_mf.h)
