cmake_minimum_required(VERSION 3.14)
project(getaddr_server)

set(CMAKE_CXX_STANDARD 17)

add_library(server STATIC
        server.h

        ioworker.h
        ioworker.cpp

        iotask.h
        iotask.cpp)

add_executable(getaddr_server
        main.cpp

        gai_task.h
        gai_task.cpp

        gai_client.h
        gai_client.cpp)

target_link_libraries(getaddr_server server)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -pedantic -pthread")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG -fsanitize=address,undefined,leak")
endif ()