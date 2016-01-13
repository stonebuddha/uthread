cmake_minimum_required(VERSION 2.6)
project(uthread)

set(TEST_SRCS test.c uthread.c uthread.h)
add_executable(test ${TEST_SRCS})
