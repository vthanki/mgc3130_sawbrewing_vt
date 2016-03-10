cmake_minimum_required(VERSION 3.0)

project(mgc3130)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-tree-ch -pthread -std=c++11")

set(SOURCE_FILES main.cpp)

add_executable(mgc3130 ${SOURCE_FILES})
