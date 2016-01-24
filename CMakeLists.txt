cmake_minimum_required(VERSION 2.8)
project(uthread)

enable_testing()

include_directories(inc)
add_subdirectory(lib)
add_subdirectory(test)
