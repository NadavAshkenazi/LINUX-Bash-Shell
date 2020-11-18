cmake_minimum_required(VERSION 3.5)
project(os_hw1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(os_hw1
        Commands.cpp
        Commands.h
        signals.cpp
        signals.h
        smash.cpp)
