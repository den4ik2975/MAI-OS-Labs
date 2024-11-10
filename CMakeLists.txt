cmake_minimum_required(VERSION 3.25)
project(OS_Labs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Опции для выбора лабораторных работ
option(BUILD_LAB1 "Build Lab 1" OFF)
option(BUILD_LAB2 "Build Lab 2" OFF)
option(BUILD_LAB3 "Build Lab 3" OFF)


# Добавление подпроектов в зависимости от опций
if(BUILD_LAB1)
    add_subdirectory(Labs/Lab1)
endif()

if(BUILD_LAB2)
    add_subdirectory(Labs/Lab2)
endif()

if(BUILD_LAB2)
    add_subdirectory(Labs/Lab3)
endif()