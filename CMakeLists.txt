cmake_minimum_required(VERSION 3.14)
project(getaddr_server)

set(CMAKE_CXX_STANDARD 17)

add_executable(getaddr_server
        main.cpp
        iojob.cpp
        iojob.h
        server.cpp
        server.h
        )