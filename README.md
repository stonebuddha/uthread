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

The thread library has a preemptive round-robin scheduler with fixed-length time slices. I use `sigaction` to register `SIGPROF` with the scheduler, and `setitimer` to set up an interval timer to send signal `SIGPROF`. I use `ucontext_t` to store the context of the thread, and the thread control block contains a stack segment and some flags such as `valid`, `exited`, `joined`, `detached` and `canceled` besides the context. My implementation just allocate `POOL_SIZE` of thread control blocks globally (in the data segment of the thread library) and privately (cannot be accessed without library functions). I use `getcontext`, `makecontext`, `setcontext` and `swapcontext` to handle the context of each thread and thread context switching.

For the round-robin scheduler, I use a FIFO queue to store the identifier of ready threads. Also, in order to make thread identifiers reusable, I use a FIFO queue to store the fresh thread identifiers, which mean identifiers that new threads can have. I initialize the thread library when the client code firstly invokes `uthread_create`. The private `uthread_init` gets information about the main thread and initializes some internal data structures. The thread routine is assumed to be the form `void *(*) (void *)`, which accepts a `void *` as an argument, and returns a `void *`. I use `thread_stub` to wrap the routine function, which simply invokes the routine and passes the returned value to `uthread_exit`.

The design of library functions is much like the famous pthread, but some of the functions have different semantics. For example, my implementation of `uthread_cancel` doesn't promise the thread is cancelled immediately. The details can be found in the following section of document. Besides essential functions, I also implement a set of synchronization mechanisms: mutex, conditional variable, and semaphore. I use a singly linked list as the waiting list that these mechanisms need, and in order to use the singly linked list in a FIFO way, I record the tail of the list as well.

At last, to avoid annoying race conditions, I use two methods. One is in the scheduler. When invoking `sigaction`, I block the signal `SIGPROF` so the scheduler won't be interrupted. The other is a global `in_critical` flag. At the beginning of all library routines, I set `in_critical` to 1, and before every return, I set `in_critical` back to 0. The scheduler checks `in_critical` firstly, and if that is 1, it then doesn't do thread context switch.

# Document

* `int uthread_create(uthread *thread, void *(*start_routine) (void *), void *arg)`

Create a new thread, starting with the execution of `start_routine` getting passed `arg`. The new identifier is stored in `*thread`.

It returns 0 on success, and `EAGAIN` if the thread pool is full.

* `void uthread_exit(void *retval)`

Terminate calling thread, and return `retval`.

It never returns, but if there's no other ready thread, the whole process exits with 0.

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
