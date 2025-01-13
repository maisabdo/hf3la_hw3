cmake_minimum_required(VERSION 3.25)
project(hw3 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(hw3
        client.c
        output.c
        request.c
        request.h
        segel.c
        segel.h
        server.c)
