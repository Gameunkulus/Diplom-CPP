cmake_minimum_required(VERSION 3.20.0)

project(ini_file)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE_LIB "ini_file.cpp" "ini_file.h")

add_library(ini_file STATIC ${SOURCE_LIB})