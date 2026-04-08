cmake_minimum_required(VERSION 3.10)
project(CrowlerApp)

add_executable(CrowlerApp
    indexer.h indexer.cpp
    main.cpp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

find_package(libpqxx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libpqxx::pqxx)

find_package(Boost REQUIRED COMPONENTS locale)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::locale)

target_link_libraries(${PROJECT_NAME} PRIVATE ini_file data_base utils)
