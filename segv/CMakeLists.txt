cmake_minimum_required(VERSION 3.2.0)

project(segv)
add_compile_options(-g)
find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})
set(EXE ${PROJECT_NAME}_practice)

add_executable(${EXE} main.cc)
target_link_libraries(${EXE} ${EIGEN_LIBRARIES} pthread)
