cmake_minimum_required(VERSION 3.14)
project(raspberry C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_C_FLAGS -pthread)

add_executable(server term_server.c)
add_executable(client term_server.c)