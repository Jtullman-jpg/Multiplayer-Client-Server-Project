cmake_minimum_required(VERSION 3.29)
project(project)

set(CMAKE_CXX_STANDARD 14)

add_executable(project
    client.c
    questions.txt
    server.c)
