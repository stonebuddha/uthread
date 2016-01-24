cmake_minimum_required(VERSION 2.8)
project(uthread)

include_directories(inc)
add_subdirectory(lib)
add_subdirectory(test)

#set(TEST_SRCS test.c)
#add_executable(test ${TEST_SRCS})
#target_link_libraries(test uthread)
