cmake_minimum_required(VERSION 3.8)

project(HttpServerApp)

add_executable(HttpServerApp 
	main.cpp
	http_connection.h
	http_connection.cpp
	)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20) 

set(libpqxx_DIR "..\\..\\libs\\libpqxx\\lib\\cmake\\libpqxx")
find_package(libpqxx REQUIRED)

target_link_libraries(${PROJECT_NAME} libpqxx::pqxx)

set(Boost_USE_STATIC_LIBS ON)       
set(Boost_USE_MULTITHREADED ON) 

set(BOOST_ROOT "..\\..\\libs\\boost")
find_package(Boost REQUIRED COMPONENTS locale)

target_link_libraries(${PROJECT_NAME} Boost::locale)

#target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})

#target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)

target_include_directories(ini_file PUBLIC "../ini_file")

target_link_libraries(${PROJECT_NAME} ini_file)

target_include_directories(data_base PUBLIC "../data_base")

target_link_libraries(${PROJECT_NAME} data_base)
