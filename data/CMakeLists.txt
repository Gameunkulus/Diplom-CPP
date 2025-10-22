cmake_minimum_required(VERSION 3.8)

project(data_base)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE_LIB "data_base.cpp" "data_base.h")

add_library(data_base STATIC ${SOURCE_LIB})

target_link_libraries(data_base PUBLIC pqxx)