cmake_minimum_required(VERSION 3.10)

project(Diplom_CPP)

add_compile_definitions(_WIN32_WINNT=0x0A01)

find_package(OpenSSL REQUIRED)

add_subdirectory(utils)
add_subdirectory(data_base)
add_subdirectory(ini_file)
add_subdirectory(CrowlerApp)
add_subdirectory(HttpServerApp)
add_subdirectory(tests)
