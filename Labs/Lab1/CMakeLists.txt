cmake_minimum_required(VERSION 3.25)
project(Lab1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=maybe-uninitialized")

# add_library(${PROJECT_NAME}_lib src/program/counter.cpp)

add_executable(${PROJECT_NAME}_exe src/parent.c)
# target_link_libraries(${PROJECT_NAME}_exe PRIVATE ${PROJECT_NAME}_lib)
add_executable(child1 src/child1.c)
add_executable(child2 src/child2.c)

set_target_properties(${PROJECT_NAME}_exe PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

