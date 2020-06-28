cmake_minimum_required(VERSION 3.2.0)

project(thread)

set(EXE ${PROJECT_NAME}_practice)

add_executable(${EXE} main.cc)
target_link_libraries(${EXE} pthread)
