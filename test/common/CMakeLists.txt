cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS *.cpp *.h)

add_library(common STATIC ${SRCS})

target_link_libraries(common server gtest_main gmock_main)
