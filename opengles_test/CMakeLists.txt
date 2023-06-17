project(gles_test)
cmake_minimum_required(VERSION 3.16)

add_executable(gles_headless_compute_shader src/gles_headless_compute_shader.cpp)

target_link_libraries(gles_headless_compute_shader -lGLESv2 -lEGL -lgbm)


add_executable(gl3_cs_basic src/gl3_cs_basic.cpp)
target_link_libraries(gl3_cs_basic -lGLESv2 -lEGL)

add_executable(gles2_triangle src/sdl_opengles2_triangle.cpp)
target_link_libraries(gles2_triangle -lSDL2)
