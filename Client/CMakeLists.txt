cmake_minimum_required(VERSION 3.21)
project(Client)

set(CMAKE_CXX_STANDARD 14)

add_executable(Client main.cpp ../TCP/packet_utils.h ../TCP/packet_utils.cpp SWClient.cpp SWClient.h)
