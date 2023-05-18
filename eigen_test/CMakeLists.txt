project(eigen_test)
cmake_minimum_required(VERSION 3.16)

# ------------------------------ eigen ---------------------------------------
find_package(Eigen3 REQUIRED)
include_directories(${Eigen3_INCLUDE_DIRS})

add_executable(eigen_test src/eigen_test.cpp)
target_link_libraries(eigen_test ${Eigen3_LIBRARIES})