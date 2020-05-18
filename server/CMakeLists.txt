cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS *.cpp *.h)

include_directories(third_party/simple_web_server)
include_directories(third_party/glog)
include_directories(third_party/jsoncpp/include)
include_directories(third_party/fmt/include)
include_directories(third_party/fmt/include)

add_library(server STATIC ${SRCS})

find_package(Boost COMPONENTS filesystem REQUIRED)

target_link_libraries(server simple-web-server glog jsoncpp_lib fmt base solver ${Boost_FILESYSTEM_LIBRARY} )
