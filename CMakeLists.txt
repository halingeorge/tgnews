cmake_minimum_required(VERSION 3.15)

project(tgnews)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)

SET(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}")

add_subdirectory(third_party)

add_subdirectory(bin)
add_subdirectory(server)
add_subdirectory(test)