cmake_minimum_required(VERSION 3.15)

enable_testing()

include_directories(
    server
    third_party/gtest
)

file(GLOB_RECURSE SRCS *.cpp *.h)

add_executable(test_server ${SRCS})

set_target_properties(test_server PROPERTIES COMPILE_FLAGS "")
target_link_libraries(test_server server gtest_main)
