cmake_minimum_required(VERSION 3.15)

project(tgnews)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=thread")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(APPLE)
else()
  set(CMAKE_CXX_FLAGS "-fPIC")
  set(Boost_USE_STATIC_LIBS ON)
  find_package(Boost COMPONENTS filesystem system)
endif()

set(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}")

set(BUILD_TESTING 0)
set(LIB_ONLY True)
add_subdirectory(third_party)
unset(LIB_ONLY)
unset(BUILD_TESTING)

add_subdirectory(bin)
add_subdirectory(server)
add_subdirectory(test)
add_subdirectory(solver)
add_subdirectory(base)
