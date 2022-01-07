cmake_minimum_required(VERSION 3.21)
project(TCP)

set(CMAKE_CXX_STANDARD 14)

add_executable(TCP main.cpp StopAndWait.cpp StopAndWait.h packet_utils.h packet_utils.cpp)
