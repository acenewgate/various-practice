cmake_minimum_required(VERSION 3.2.0)

project(various_practice)

add_compile_options(-std=c++11 -O3 -g)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/binary)
add_subdirectory(thread)
add_subdirectory(segv)
