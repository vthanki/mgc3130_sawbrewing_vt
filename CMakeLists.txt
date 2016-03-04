cmake_minimum_required(VERSION 3.0)

#option(USE_WiringPi "Use gpio/serial wiring library" ON)

project(mgc3130)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-tree-ch -pthread -std=c++11")

set(SOURCE_FILES main.cpp)

add_executable(mgc3130 ${SOURCE_FILES})
    set_target_properties(mgc3130 PROPERTIES COMPILE_DEFINITIONS "WIRING_PI=1")
    target_link_libraries (mgc3130 wiringPi)
