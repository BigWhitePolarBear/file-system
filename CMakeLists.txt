cmake_minimum_required(VERSION 3.21)
project(file-system)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-g -Wall -Werror")
set(CMAKE_C_FLAGS_DEBUG "-O0")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(CMAKE_INSTALL_PREFIX "install")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/build/bin)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)
