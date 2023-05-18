project(sophus_test)
cmake_minimum_required(VERSION 3.16)


# ------------------------------ sophus ---------------------------------------
find_package(Sophus REQUIRED)
include_directories(${Sophus_INCLUDE_DIRS} ${Eigen3_INCLUDE_DIRS})

add_executable(sophus_test src/sophus_test.cpp)
target_link_libraries(sophus_test ${Sophus_LIBRARIES}${Eigen3_LIBRARIES} )
