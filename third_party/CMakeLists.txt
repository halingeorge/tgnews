cmake_minimum_required(VERSION 3.15)

SET(BUILD_TESTING OFF)

include(ExternalProject)
install(SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/executors/autogen.sh)
ExternalProject_Add(executors
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/executors
        PATCH_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/executors/autogen.sh
        CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/executors/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND ${MAKE}
        INSTALL_COMMAND make install)

SET(EXECUTORS_LIB_DIR third_party/executors/src)

add_library(executors_lib INTERFACE)

add_dependencies(executors_lib executors)

set_target_properties(executors_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/executors/include)

add_subdirectory(fmt)
add_subdirectory(eigen)
add_subdirectory(tinyxml2)
add_subdirectory(fastText)
add_subdirectory(gtest)
add_subdirectory(gflags)
add_subdirectory(glog)
add_subdirectory(jsoncpp)
add_subdirectory(simple_web_server)
add_subdirectory(onmt_tokenizer)
