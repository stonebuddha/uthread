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

It returns 0 on success, and `EAGAIN` if the thread pool is full.

* `void uthread_exit(void *retval)`

Terminate calling thread, and return `retval`.

It will never return, but if there's no other ready thread, the whole process exits with 0.

* `int uthread_join(uthread_t thread, void **retval)`

Make calling thread wait for termination of the thread `thead`. The exit status of the thread is stored in `*retval`, if `retval` is not NULL. Note that one thread can be joined at most one time.

It returns 0 on success, `ESRCH` if the identifier is illegal, `EINVAL` if the thread has been joined or detached, and `EDEADLK` if there's no other ready threads. Note if all ready threads are cancelled, the whole process exits with `EDEADLK`.

* `int uthread_detach(uthread_t thread)`

Indicate that the thread `thread` is never to be joined with `uthread_join`. The resources of `thread` will therefore be freed immediately when it terminates, instead of waiting for another thread to perform `uthread_join`. Noth that a joined thread can not be detached.

It returns 0 on success, `ESRCH` if the identifier is illegal, and `EINVAL` if the thread has been joined or detached.

* `uthread_t uthread_self(void)`

Obtain the identifier of the current thread.

* `int uthread_equal(uthread_t t1, uthread_t t2)`

Compare two thread identifiers.

* `void uthread_yield(void)`

Yield the processor to another thread. Note if there's no other ready threads, the calling thread will keep running.

* `int uthread_cancel(uthread_t thread)`

Cancel the thread `thread`. Note the cancellation is not immediate. For example, if the thread pool is full and you cancel one thread, there may not be a fresh identifier immediately.

It returns 0 on success, and `ESRCH` if the identifier is illegal.

* `int uthread_mutex_init(uthread_mutex_t *mutex)`

Initialize a mutex.

* `int uthread_mutex_trylock(uthread_mutex_t *mutex)`

Try locking a mutex.

It returns 0 on success, and -1 on failure.

* `int uthread_mutex_lock(uthread_mutex_t *mutex)`

Lock a mutex.

It returns 0 in success, and `EDEADLK` if there's no other ready threads. Note if all ready threads are cancelled, the whole process exits with `EDEADLK`.

* `int uthread_mutex_unlock(uthread_mutex_t *mutex)`

Unlock a mutex.

* `int uthread_cond_init(uthread_cond_t *cond)`

Initialize a conditional variable.

* `int uthread_cond_signal(uthread_cond_t *cond)`

Wake up one thread waiting for conditional variable `cond`.

* `int uthread_cond_broadcast(uthread_cond_t *cond)`

Wake up all threads waiting for conditional variables `cond`.

* `int uthread_cond_wait(uthread_cond_t *cond, uthread_mutex_t *mutex)`

Wait for conditional variable `cond` to be signaled of broadcast. `mutex` is assumed to be locked before. Before waiting, the `mutex` is unlocked automatically and after waiting, the `mutex` is locked automatically.

It returns 0 on success, otherwise the whole process exits with `EDEADLK`.

* `int uthread_sem_init(uthread_sem_t *sem, long cnt)`

Initialize a semaphore with count `cnt`.

* `int uthread_sem_down(uthread_sem_t *sem)`

Perform a down (P) on a semaphore.

It returns 0 in success, and `EDEADLK` if there's no other ready threads. Note if all ready threads are cancelled, the whole process exits with `EDEADLK`.

* `int uthread_sem_up(uthread_sem_t *sem)`

Perform an up (V) on a semaphore.
