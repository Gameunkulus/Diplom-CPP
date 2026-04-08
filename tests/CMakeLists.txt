cmake_minimum_required(VERSION 3.10)
project(SearchEngineTests)

find_package(GTest CONFIG REQUIRED)

add_executable(tests
    test_main.cpp
)

target_compile_features(tests PRIVATE cxx_std_20)

target_link_libraries(tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    ini_file
    utils
)

find_package(Boost REQUIRED COMPONENTS locale)
target_link_libraries(tests PRIVATE Boost::locale)

include(GoogleTest)
gtest_discover_tests(tests)
