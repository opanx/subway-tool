cmake_minimum_required(VERSION 3.18)
project(subway_tool C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra -Wno-unused-parameter")

add_executable(subway_tool
    src/subway_cheat.c
)

target_link_libraries(subway_tool
    log
    pthread
    dl
)

# Strip debug symbols for smaller binary
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
