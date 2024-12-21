cmake_minimum_required(VERSION 3.10)
project(os_labs)
set(CUR_PR "lab5_7")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${CUR_PR}_lib src/lib.cpp)

add_executable(control src/control.cpp)
add_executable(computing src/computing.cpp)
target_include_directories(${CUR_PR}_lib PUBLIC src)

target_link_libraries(control PRIVATE ${CUR_PR}_lib zmq)
target_link_libraries(computing PRIVATE ${CUR_PR}_lib zmq)