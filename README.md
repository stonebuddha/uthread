User-Level Thread Library
=========================

User-level thread is one kind of threads, in constrast to kernel-level thread. Since the operating system normally doesn't know the presence of user-level thread library, the functions can be more flexibly designed. Moreover, the context switch of threads can be much easier and so much faster. However, the unawareness of operating system also sets a limit: if one user-level thread blocks, then the whole process blocks.

# Platform

I implement uthread in C under Linux ubuntu 3.16.0-30-generic #40~14.04.1-Ubuntu SMP, using CMake to manage the project.

# Usage

It's easy to build with CMake:

* `mkdir build`
* `cd build`
* `cmake ..`
* `make`
* `make test`

Then there is `libuthread.a` library, you can use the library by including `inc/uthread.h` and then linking with `libuthread.a`.

# Document

## `uthread_create(uthread *thread, void *(*start_routine) (void *), void *arg)`
