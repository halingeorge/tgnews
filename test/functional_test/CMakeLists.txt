cmake_minimum_required(VERSION 3.15)

enable_testing()

include_directories(
    server
    third_party/gtest
)

file(GLOB_RECURSE SRCS *.cpp *.h)

add_executable(functional_test ${SRCS})

set_target_properties(functional_test PROPERTIES COMPILE_FLAGS "")
target_link_libraries(functional_test base common server gtest_main gmock_main)
