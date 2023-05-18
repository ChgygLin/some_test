project(g2o_test)
cmake_minimum_required(VERSION 3.16)


# ------------------------------ g2o ---------------------------------------
# https://github.com/RainerKuemmerle/g2o
#add_subdirectory(g2o)

find_package(OpenCV REQUIRED)
include_directories(${OPENCV_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/g2o)

add_executable(g2o_test src/g2o_test.cpp)
#target_link_libraries(g2o_test ${OpenCV_LIBS})

target_link_libraries(g2o_test ${PROJECT_SOURCE_DIR}/g2o/lib/libg2o.so ${OpenCV_LIBS})