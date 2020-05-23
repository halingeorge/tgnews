cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS *.cpp *.h)

include_directories(third_party)
include_directories(third_party/fastText)
include_directories(third_party/eigen)

add_library(solver STATIC ${SRCS})

target_link_libraries(solver base eigen fasttext-static tinyxml2 OpenNMTTokenizer)
