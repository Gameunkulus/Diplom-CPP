cmake_minimum_required(VERSION 3.20)


add_executable(CrowlerApp
	main.cpp
	http_utils.h
	http_utils.cpp
	link.h
	)

target_compile_features(CrowlerApp PRIVATE cxx_std_17) 

target_link_libraries(CrowlerApp pqxx)

target_include_directories(CrowlerApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(CrowlerApp ${Boost_LIBRARIES})

target_link_libraries(CrowlerApp OpenSSL::SSL)

target_include_directories(ini_file PUBLIC "../ini_file")

target_link_libraries(CrowlerApp)

target_include_directories(data_base PUBLIC "../data_base")

target_link_libraries(CrowlerApp data_base)