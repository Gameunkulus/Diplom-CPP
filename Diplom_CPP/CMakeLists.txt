

add_executable (Diplom_CPP "Diplom_CPP.cpp" "Diplom_CPP.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Diplom_CPP PROPERTY CXX_STANDARD 20)
endif()

