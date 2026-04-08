cmake_minimum_required(VERSION 3.10)
project(utils)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(utils STATIC utils.h utils.cpp)

find_package(Boost REQUIRED COMPONENTS locale)
target_link_libraries(utils PUBLIC Boost::locale)
