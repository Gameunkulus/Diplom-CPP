cmake_minimum_required(VERSION 3.10)
project(HttpServerApp)

add_executable(HttpServerApp
    main.cpp
    http_connection.h
    http_connection.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

find_package(libpqxx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libpqxx::pqxx)

find_package(Boost REQUIRED COMPONENTS locale)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::locale)

target_link_libraries(${PROJECT_NAME} PRIVATE ini_file data_base utils)

target_link_libraries(${PROJECT_NAME} PRIVATE wsock32 ws2_32)
