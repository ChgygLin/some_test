cmake_minimum_required(VERSION 3.12)

project(my_module)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
find_package(pybind11 REQUIRED)

add_library(my_module MODULE src/my_module.cpp)
target_link_libraries(my_module PRIVATE ${OpenCV_LIBS} pybind11::module)

set_target_properties(my_module PROPERTIES
    PREFIX ""
    SUFFIX ".so"
)