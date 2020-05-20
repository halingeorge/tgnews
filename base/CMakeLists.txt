cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS *.cpp *.h)

include_directories(third_party/fmt/include)
include_directories(third_party/gflags)
include_directories(third_party/glog)
include_directories(third_party/fastText)
include_directories(third_party/onmt_tokenizer/include)
include_directories(third_party/jsoncpp/include)
include_directories(third_party/eigen)

add_library(base STATIC ${SRCS})

find_package(Boost COMPONENTS filesystem REQUIRED)

target_link_libraries(base glog gflags eigen fasttext-static OpenNMTTokenizer jsoncpp_lib fmt ${Boost_FILESYSTEM_LIBRARY})
