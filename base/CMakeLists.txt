cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS *.cpp *.h)

include_directories(third_party/fmt/include)
include_directories(third_party/gflags)
include_directories(third_party/glog)
include_directories(third_party/fastText)
include_directories(third_party/onmt_tokenizer/include)
include_directories(third_party/eigen)
include_directories(third_party/executors/include)
include_directories(third_party/continuable/include)

add_library(base STATIC ${SRCS})

find_package(Boost COMPONENTS filesystem REQUIRED)

target_link_libraries(base continuable executors_lib glog gflags eigen fasttext-static OpenNMTTokenizer fmt)

if(APPLE)
    target_link_libraries(base ${Boost_FILESYSTEM_LIBRARY})
else()
    target_link_libraries(base ${Boost_LIBRARIES})
endif()

add_dependencies(base executors)
