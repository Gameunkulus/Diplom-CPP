# CMakeList.txt: главный файл проекта CMake;
cmake_minimum_required (VERSION 3.8)


if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(Diplom_CPP)

set(OPENSSL_ROOT_DIR "C:\\Program Files\\OpenSSL-Win64")

set(BOOST_ROOT "..\\boost_1_80_0")

set(libpqxx_DIR "C:\\Program Files\\libpqxx-7.10.3")

add_compile_definitions(_WIN32_WINNT=0x0A01)

# PQXX
set(PostgreSQL_ADDITIONAL_VERSIONS "15")

add_subdirectory(${libpqxx_DIR} libpqxx-build) 

# OpenSSL
find_package(OpenSSL REQUIRED)

add_subdirectory(data_base)

add_subdirectory(ini_file)

add_subdirectory(CrowlerApp)

add_subdirectory(HttpServerApp)