cmake_minimum_required(VERSION 3.8)

project(data_base)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(libpqxx REQUIRED)

set(SOURCE_LIB "data_base.cpp" "data_base.h")

add_library(${PROJECT_NAME} STATIC ${SOURCE_LIB})

target_link_libraries(${PROJECT_NAME} PRIVATE libpqxx::pqxx)
