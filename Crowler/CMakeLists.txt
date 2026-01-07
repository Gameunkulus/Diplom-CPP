cmake_minimum_required(VERSION 3.8)

project(CrowlerApp)

set(CMAKE_CXX_STANDARD 20)

add_executable(CrowlerApp
	main.cpp
	http_utils.h
	http_utils.cpp
	link.h
	)
	
set(libpqxx_DIR "..\\..\\libs\\libpqxx\\lib\\cmake\\libpqxx")
find_package(libpqxx REQUIRED)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20) 

target_link_libraries(${PROJECT_NAME} libpqxx::pqxx)

set(Boost_USE_STATIC_LIBS ON)       
set(Boost_USE_MULTITHREADED ON) 

set(BOOST_ROOT "..\\..\\libs\\boost")
find_package(Boost REQUIRED COMPONENTS locale)
message(STATUS "boost include is ${Boost_INCLUDE_DIRS}")
message(STATUS "boost lib is ${Boost_LIBRARY_DIRS}")

#target_include_directories(CrowlerApp PRIVATE ${Boost_INCLUDE_DIRS})

#target_link_libraries(CrowlerApp ${Boost_LIBRARY_DIRS})
# Правильный способ линковки:
target_link_libraries(${PROJECT_NAME} Boost::locale)

#target_compile_definitions(CrowlerApp PRIVATE BOOST_ALL_NO_LIB)

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)

target_include_directories(ini_file PUBLIC "../ini_file")

target_link_libraries(${PROJECT_NAME} ini_file)

target_include_directories(data_base PUBLIC "../data_base")

target_link_libraries(${PROJECT_NAME} data_base)

#target_link_libraries(CrowlerApp libpqxx::pqxx)