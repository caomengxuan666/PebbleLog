cmake_minimum_required(VERSION 3.25)

project(PebbleLog VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include_directories(PebbleLog)

add_subdirectory(PebbleLog)

add_subdirectory(example)