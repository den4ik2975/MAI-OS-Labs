cmake_minimum_required(VERSION 3.10)
project(distributed_computing)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src/include)

# Create library from common source files
add_library(common_lib
        src/nodes.cpp
        src/utils.cpp
        src/zmq_operations.cpp
)

# Set header include directories for the library
target_include_directories(common_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Create executables
add_executable(control src/control.cpp)
add_executable(computing src/child.cpp)

# Link libraries
target_link_libraries(control
        PRIVATE
        common_lib
        zmq
)

target_link_libraries(computing
        PRIVATE
        common_lib
        zmq
)

# Set compile options if needed
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(control PRIVATE -Wall -Wextra)
    target_compile_options(computing PRIVATE -Wall -Wextra)
endif()
