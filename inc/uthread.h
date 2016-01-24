//
// Created by Wayne on 16/1/13.
//

#ifndef UTHREAD_H
#define UTHREAD_H


#define UTHREAD_CANCELED    ((void *) -1)

#define UTHREAD_MUTEX_SIZE  24
#define UTHREAD_COND_SIZE   16
#define UTHREAD_SEM_SIZE    24

typedef unsigned long int uthread_t;


/* Create a new thread, starting with execution of START-ROUTINE
 * getting passed ARG. The new identifier is stored in *THREAD.
 */
int uthread_create(uthread_t *thread, void *(*start_routine) (void *), void *arg);

/* Terminate calling thread.
 */
void uthread_exit(void *retval);

/* Make calling thread wait for termination of the thread THREAD. The
 * exit status of the thread is stored in *RETVAL, if RETVAL is not NULL.
 */
int uthread_join(uthread_t thread, void **retval);

/* Indicate that the thread THREAD is never to be joined with UTHREAD_JOIN.
 * The resources of THREAD will therefore be freed immediately when it
 * terminates, instead of waiting for another thread to perform UTHREAD_JOIN
 * on it.
 */
int uthread_detach(uthread_t thread);

/* Obtain the identifier of the current thread.
 */
uthread_t uthread_self(void);

/* Compare two thread identifiers.
 */
int uthread_equal(uthread_t t1, uthread_t t2);

/* Yield the processor to another thread.
 */
void uthread_yield(void);

/* Cancel THREAD (not immediately).
 */
int uthread_cancel(uthread_t thread);


/* Mutex handling.
 */

typedef struct {
    char span[UTHREAD_MUTEX_SIZE];
} uthread_mutex_t;

/* Initialize a mutex.
 */
int uthread_mutex_init(uthread_mutex_t *mutex);

/* Try locking a mutex.
 */
int uthread_mutex_trylock(uthread_mutex_t *mutex);

/* Lock a mutex.
 */
int uthread_mutex_lock(uthread_mutex_t *mutex);

/* Unlock a mutex.
 */
int uthread_mutex_unlock(uthread_mutex_t *mutex);


/* Conditional variable handling.
 */

typedef struct {
    char span[UTHREAD_COND_SIZE];
} uthread_cond_t;

/* Initialize a conditional variable.
 */
int uthread_cond_init(uthread_cond_t *cond);

/* Wake up one thread waiting for conditional variable COND.
 */
int uthread_cond_signal(uthread_cond_t *cond);

/* Wake up all threads waiting for conditional variables COND.
 */
int uthread_cond_broadcast(uthread_cond_t *cond);

/* Wait for conditional variable COND to be signaled or broadcast.
 * MUTEX is assumed to be locked before.
 */
int uthread_cond_wait(uthread_cond_t *cond, uthread_mutex_t *mutex);


/* Semaphore handling.
 */

typedef struct {
    char span[UTHREAD_SEM_SIZE];
} uthread_sem_t;

/* Initialize a semaphore.
 */
int uthread_sem_init(uthread_sem_t *sem, long cnt);

/* Down a semaphore.
 */
int uthread_sem_down(uthread_sem_t *sem);

/* Up a semaphore.
 */
int uthread_sem_up(uthread_sem_t *sem);

#endif
