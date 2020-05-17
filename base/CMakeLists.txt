cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS *.cpp *.h)

add_library(base STATIC ${SRCS})

find_package(Boost COMPONENTS filesystem REQUIRED)

target_link_libraries(base ${Boost_FILESYSTEM_LIBRARY})
