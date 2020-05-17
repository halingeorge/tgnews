cmake_minimum_required(VERSION 3.15)

enable_testing()

include_directories(
    server
    third_party/gtest
)

file(GLOB_RECURSE SRCS *.cpp *.h)

add_executable(stress_test ${SRCS})

set_target_properties(stress_test PROPERTIES COMPILE_FLAGS "")
target_link_libraries(stress_test base server gtest_main gmock_main)
