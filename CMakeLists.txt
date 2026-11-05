cmake_minimum_required(VERSION 4.3.1)

project(zormid)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(zormid ${SRC_FILES})

target_include_directories(zormid PUBLIC src)
target_include_directories(zormid PUBLIC lib)