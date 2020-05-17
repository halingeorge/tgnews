cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

include_directories(third_party/gflags)
include_directories(third_party/glog)

add_executable(tgnews main.cpp)

target_link_libraries(tgnews server gflags glog solver base)
