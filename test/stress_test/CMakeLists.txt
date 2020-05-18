cmake_minimum_required(VERSION 3.15)

include_directories(server)
include_directories(third_party/gflags)
include_directories(third_party/glog)

add_executable(stress_test stress_test.cpp)

target_link_libraries(stress_test base common gflags glog fmt simple-web-server)
