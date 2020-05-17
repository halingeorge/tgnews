cmake_minimum_required(VERSION 3.15)

enable_testing()

include_directories(
    server
    third_party/gtest
)

file(GLOB_RECURSE SRCS *.cpp *.h)

add_executable(unit_test ${SRCS})

set_target_properties(unit_test PROPERTIES COMPILE_FLAGS "")
target_link_libraries(unit_test server gtest_main)
