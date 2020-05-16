cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS *.cpp *.h)

include_directories(third_party)

add_library(solver ${SRCS})

target_link_libraries(solver tinyxml2)
