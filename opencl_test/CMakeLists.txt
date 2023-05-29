project(opencl_test)
cmake_minimum_required(VERSION 3.16)

# ------------------------------ opencl ---------------------------------------
find_package(OpenCL REQUIRED)

add_executable(vector_add src/vector_add.cpp)
target_link_libraries(vector_add OpenCL::OpenCL)

add_executable(matrix_multiply src/matrix_multiply.cpp)
target_link_libraries(matrix_multiply OpenCL::OpenCL)

find_package(OpenCV REQUIRED)
add_executable(rotate src/rotate.cpp)
target_link_libraries(rotate OpenCL::OpenCL ${OpenCV_LIBS})

add_executable(remap src/remap.cpp)
target_link_libraries(remap OpenCL::OpenCL ${OpenCV_LIBS})


