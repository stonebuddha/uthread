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

Then there is a `libuthread.a` library, you can use the library by including `inc/uthread.h` and then linking with `libuthread.a`.

# Design

# Implementation

# Document

* `int uthread_create(uthread *thread, void *(*start_routine) (void *), void *arg)`
Create a new thread, starting with the execution of `start_routine` getting passed `arg`. The new identifier is stored in `*thread`.

* `void uthread_exit(void *retval)`
Terminate calling thread, and return `retval`.

* `int uthread_join(uthread_t thread, void **retval)`
Make calling thread wait for termination of the thread `thead`. The exit status of the thread is stored in `*retval`, if `retval` is not NULL. Note that one thread can be joined at most one time.

* `int uthread_detach(uthread_t thread)`
Indicate that the thread `thread` is never to be joined with `uthread_join`. The resources of `thread` will therefore be freed immediately when it terminates, instead of waiting for another thread to perform `uthread_join`. Noth that a joined thread can not be detached.

* `uthread_t uthread_self(void)`
Obtain the identifier of the current thread.

* `int uthread_equal(uthread_t t1, uthread_t t2)`
Compare two thread identifiers.

* `void uthread_yield(void)`
Yield the processor to another thread. Note if there's no other ready threads, the calling thread will keep running.

* `int uthread_cancel(uthread_t thread)`
Cancel the thread `thread`. Note the cancellation is not immediate. For example, if the thread pool is full and you cancel one thread, there may not be a fresh identifier immediately.

* `int uthread_mutex_init(uthread_mutex_t *mutex)`
Initialize a mutex.

* `int uthread_mutex_trylock(uthread_mutex_t *mutex)`
Try locking a mutex.

* `int uthread_mutex_lock(uthread_mutex_t *mutex)`
Lock a mutex.

* `int uthread_mutex_unlock(uthread_mutex_t *mutex);
Unlock a mutex.
