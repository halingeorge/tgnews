cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS *.cpp *.h)

include_directories(../third_party/simple_web_server)

add_library(server ${SRCS})

target_link_libraries(server simple-web-server)