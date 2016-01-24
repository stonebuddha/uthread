cmake_minimum_required(VERSION 2.6)
project(uthread)

set(UTHREAD_SRCS uthread.c uthread.h)
add_library(uthread ${UTHREAD_SRCS})

set(TEST_SRCS test.c)
add_executable(test ${TEST_SRCS})
target_link_libraries(test uthread)
