//
// Created by Wayne on 16/1/13.
//

#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include "uthread.h"

/* Constants.
 */

#define STACK_SIZE 4096
#define POOL_SIZE 1024
#define TIME_SLICE 10000


/* Thread control block.
 */

typedef struct {
    ucontext_t uc;              // saved context
    void *retval;               // pointer to returned value
    uthread_t joined;           // joiner
    void **joined_retval;       // pointer to pointer to returned value of joiner
    char valid;                 // valid flag
    char exited;                // exited flag
    char detached;              // detached flag
    char canceled;              // canceled flag
    char stack[STACK_SIZE];     // stack segment
} thread_control_block;


/* Internal data structures.
 */

static thread_control_block tcbs[POOL_SIZE];    // static alloced thread control blocks
static char inited = 0;                         // inited flag
static uthread_t current_thread_id;             // identifier of current running thread
static struct itimerval time_slice;             // time slice
static struct sigaction scheduler;              // sigaction for scheduler
static char in_critical = 0;                    // in-critical flag


/* Free thread identifier queue.
 */

static uthread_t free_tids[POOL_SIZE + 1];
static int free_tids_head = 0;
static int free_tids_tail = 0;

/* Get a fresh thread identifier.
 */
static uthread_t get_fresh_tid() {
    if (free_tids_head == free_tids_tail) {
        return POOL_SIZE;
    } else {
        uthread_t res;

        res = free_tids[free_tids_head];
        free_tids_head = (free_tids_head + 1) % (POOL_SIZE + 1);

        return res;
    }
}

/* Put back a thread identifier to be fresh.
 */
static void put_fresh_tid(uthread_t tid) {
    free_tids[free_tids_tail] = tid;
    free_tids_tail = (free_tids_tail + 1) % (POOL_SIZE + 1);
}

/* Check if there is no fresh identifiers.
 */
static int is_fresh_queue_empty() {
    return (free_tids_head == free_tids_tail);
}


/* Ready thread identifier queue.
 */

static uthread_t ready_queue[POOL_SIZE + 1];
static int ready_queue_head = 0, ready_queue_tail = 0;

/* Get a ready thread identifier.
 */
static uthread_t get_ready_tid() {
    if (ready_queue_head == ready_queue_tail) {
        return POOL_SIZE;
    } else {
        uthread_t res;

        res = ready_queue[ready_queue_head];
        ready_queue_head = (ready_queue_head + 1) % (POOL_SIZE + 1);

        return res;
    }
}

/* Put back a ready thread identifier.
 */
static void put_ready_tid(uthread_t tid) {
    ready_queue[ready_queue_tail] = tid;
    ready_queue_tail = (ready_queue_tail + 1) % (POOL_SIZE + 1);
}

/* Check if there is no ready identifiers.
 */
static int is_ready_queue_empty() {
    return (ready_queue_head == ready_queue_tail);
}


/* Important internal functions.
 */

/* A round-robin scheduler, implemented as a signal controller.
 */
static void schedule(int sig) {
    // if the current thread is in library functions or there's no ready thread, do nothing
    if (in_critical || is_ready_queue_empty()) {
        return;
    } else {
        uthread_t previous_thread_id;
        int flag = 0;

        put_ready_tid(current_thread_id);
        previous_thread_id = current_thread_id;
        while (!is_ready_queue_empty()) {
            current_thread_id = get_ready_tid();
            
            // deal with the canceled threads
            if (!tcbs[current_thread_id].canceled) {
                flag = 1;
                break;
            } else {
                // if the thread is detached, simply set the valid flag
                if (tcbs[current_thread_id].detached) {
                    tcbs[current_thread_id].valid = 0;
                } else {
                    tcbs[current_thread_id].retval = UTHREAD_CANCELED;
                    tcbs[current_thread_id].exited = 1;

                    if (tcbs[current_thread_id].joined != POOL_SIZE) {
                        tcbs[current_thread_id].valid = 0;

                        if (tcbs[current_thread_id].joined_retval != NULL) {
                            (*tcbs[current_thread_id].joined_retval) = UTHREAD_CANCELED;
                        }
                        put_ready_tid(tcbs[current_thread_id].joined);
                    }
                }
            }
        }

        if (flag) {
            swapcontext(&tcbs[previous_thread_id].uc, &tcbs[current_thread_id].uc);
        }
    }
}

/* A stub function for UTHREAD_CREATE, used to wrap START-ROUTINE.
 */
static void thread_stub(void *(*start_routine) (void *), void *arg) {
    uthread_exit(start_routine(arg));
}

/* Initialize internal data structures.
 */
static int uthread_init() {
    uthread_t tid;
    uthread_t main_thread_id;
    thread_control_block *tcb;
    int tmp;

    for (tid = 0; tid < POOL_SIZE; ++ tid) {
        tcbs[tid].valid = 0;
        put_fresh_tid(tid);
    }

    // set the signal handler as the scheduler, and
    // the scheduler itselt cannot be interrupted
    scheduler.sa_handler = schedule;
    scheduler.sa_flags = SA_RESTART;
    sigemptyset(&scheduler.sa_mask);
    sigaddset(&scheduler.sa_mask, SIGPROF);
    if ((tmp = sigaction(SIGPROF, &scheduler, NULL)) != 0) {
        return tmp;
    }

    // set up the main thread
    main_thread_id = get_fresh_tid();
    if (main_thread_id == POOL_SIZE) {
        return -1;
    }
    tcb = &tcbs[main_thread_id];
    getcontext(&tcb->uc);
    tcb->joined = POOL_SIZE;
    tcb->valid = 1;
    tcb->exited = 0;
    tcb->detached = 1;
    current_thread_id = main_thread_id;

    // set up time slice, using SETITIMER
    time_slice.it_value.tv_sec = 0;
    time_slice.it_value.tv_usec = TIME_SLICE;
    time_slice.it_interval = time_slice.it_value;
    if ((tmp = setitimer(ITIMER_PROF, &time_slice, NULL)) != 0) {
        return tmp;
    }

    inited = 1;

    return 0;
}


/* User-level thread library functions.
 */

/* Create a new thread, starting with execution of START-ROUTINE
 * getting passed ARG. The new handle is stored in *THREAD.
 */
int uthread_create(uthread_t *thread, void *(*start_routine) (void *), void *arg) {
    thread_control_block *tcb;
    uthread_t tid;

    in_critical = 1;

    if (!inited) {
        int init_res = uthread_init();
        if (init_res != 0) {
            in_critical = 0;
            return init_res;
        }
    }

    // get a fresh identifier
    if (is_fresh_queue_empty()) {
        in_critical = 0;
        return EAGAIN;
    }
    tid = get_fresh_tid();
    if (thread != NULL) {
        (*thread) = tid;
    }

    // initialize the thread control block
    tcb = &tcbs[tid];
    getcontext(&tcb->uc);
    tcb->uc.uc_stack.ss_sp = (void *) tcb->stack;
    tcb->uc.uc_stack.ss_size = STACK_SIZE;
    tcb->joined = POOL_SIZE;
    tcb->valid = 1;
    tcb->exited = 0;
    tcb->detached = 0;
    makecontext(&tcb->uc, (void (*) (void)) thread_stub, 2, start_routine, arg);

    put_ready_tid(tid);

    in_critical = 0;
    return 0;
}

/* Terminate calling thread.
 */
void uthread_exit(void *retval) {
    in_critical = 1;

    // if the thread is detached, simply set the valid flag
    // otherwise wake up the joiner
    if (tcbs[current_thread_id].detached) {
        tcbs[current_thread_id].valid = 0;
    } else {
        tcbs[current_thread_id].retval = retval;
        tcbs[current_thread_id].exited = 1;

        if (tcbs[current_thread_id].joined != POOL_SIZE) {
            tcbs[current_thread_id].valid = 0;

            if (tcbs[current_thread_id].joined_retval != NULL) {
                (*tcbs[current_thread_id].joined_retval) = retval;
            }
            put_ready_tid(tcbs[current_thread_id].joined);
        }
    }

    if (is_ready_queue_empty()) {
        exit(0);
    } else {
        current_thread_id = get_ready_tid();

        in_critical = 0;
        setcontext(&tcbs[current_thread_id].uc);
    }
}

/* Make calling thread wait for termination of the thread THREAD. The
 * exit status of the thread is stored in *RETVAL, if RETVAL is not NULL.
 */
int uthread_join(uthread_t thread, void **retval) {
    in_critical = 1;

    if (thread >= POOL_SIZE || !tcbs[thread].valid) {
        in_critical = 0;
        return ESRCH;
    } else {
        if (!tcbs[thread].exited) {
            if (tcbs[thread].joined != POOL_SIZE) {
                in_critical = 0;
                return EINVAL;
            } else if (tcbs[thread].detached) {
                in_critical = 0;
                return EINVAL; 
            } else {
                if (is_ready_queue_empty()) {
                    in_critical = 0;
                    return EDEADLK;
                } else {
                    uthread_t previous_thread_id;

                    // set the joiner in the thread control block of joinee
                    tcbs[thread].joined = current_thread_id;
                    tcbs[thread].joined_retval = retval;

                    previous_thread_id = current_thread_id;
                    current_thread_id = get_ready_tid();

                    in_critical = 0;
                    swapcontext(&tcbs[previous_thread_id].uc, &tcbs[current_thread_id].uc);
                    return 0;
                }
            }
        } else {
            if (retval != NULL) {
                (*retval) = tcbs[thread].retval;
            }
            
            in_critical = 0;
            return 0;
        }
    }
}

/* Indicate that the thread THREAD is never to be joined with UTHREAD_JOIN.
 * The resources of THREAD will therefore be freed immediately when it
 * terminates, instead of waiting for another thread to perform UTHREAD_JOIN
 * on it.
 */
int uthread_detach(uthread_t thread) {
    in_critical = 1;

    if (thread >= POOL_SIZE || !tcbs[thread].valid) {
        in_critical = 0;
        return ESRCH;
    } else {
        if (tcbs[thread].detached || tcbs[thread].joined != POOL_SIZE) {
            in_critical = 0;
            return EINVAL;
        } else {
            tcbs[thread].detached = 1;

            in_critical = 0;
            return 0;
        }
    }
}

/* Obtain the identifier of the current thread.
 */
uthread_t uthread_self() {
    return current_thread_id;
}

/* Compare two thread identifiers.
 */
int uthread_equal(uthread_t t1, uthread_t t2) {
    return (t1 == t2);
}

/* Yield the processor to another thread.
 */
void uthread_yield() {
    in_critical = 1;

    // the code is like SCHEDULE
    if (is_ready_queue_empty()) {
        in_critical = 0;
        return;
    } else {
        uthread_t previous_thread_id;
        int flag = 0;

        put_ready_tid(current_thread_id);
        previous_thread_id = current_thread_id;
        while (!is_ready_queue_empty()) {
            current_thread_id = get_ready_tid();
            
            // deal with the canceled threads
            if (!tcbs[current_thread_id].canceled) {
                flag = 1;
                break;
            } else {
                // if the thread is detached, simply set the valid flag
                if (tcbs[current_thread_id].detached) {
                    tcbs[current_thread_id].valid = 0;
                } else {
                    tcbs[current_thread_id].retval = UTHREAD_CANCELED;
                    tcbs[current_thread_id].exited = 1;

                    if (tcbs[current_thread_id].joined != POOL_SIZE) {
                        tcbs[current_thread_id].valid = 0;

                        if (tcbs[current_thread_id].joined_retval != NULL) {
                            (*tcbs[current_thread_id].joined_retval) = UTHREAD_CANCELED;
                        }
                        put_ready_tid(tcbs[current_thread_id].joined);
                    }
                }
            }
        }

        if (flag) {
            in_critical = 0;
            swapcontext(&tcbs[previous_thread_id].uc, &tcbs[current_thread_id].uc);
        } else {
            in_critical = 0;
        }

        return;
    }
}

/* Cancel THREAD immediately.
 */
int uthread_cancel(uthread_t thread) {
    in_critical = 1;

    if (thread >= POOL_SIZE || !tcbs[thread].valid) {
        in_critical = 0;
        return ESRCH;
    } else {
        // simply set the canceled flag
        tcbs[thread].canceled = 1;
        
        in_critical = 0;
        return 0;
    }
}


/* Internal linked list.
 */

typedef struct _linked_list_t {
    uthread_t tid;
    void *data;
    struct _linked_list_t *next;
} linked_list_t;

/* Put on the tail of the linked list.
 */
static void linked_list_put_tail(linked_list_t **des, linked_list_t **tail, uthread_t tid, void *data) {
    linked_list_t *to_ins = (linked_list_t *) malloc(sizeof(linked_list_t));

    to_ins->tid = tid;
    to_ins->data = data;
    to_ins->next = NULL;

    if ((*des) == NULL) {
        (*des) = to_ins;
        (*tail) = to_ins;
    } else {
        des = &((*tail)->next);
        (*des) = to_ins;
        (*tail) = to_ins;
    }
}

/* Get the head of the linked list.
 */
static void linked_list_get_head(linked_list_t **des, linked_list_t **tail, uthread_t *tid, void **data) {
    linked_list_t *to_del;

    to_del = (*des);
    if (tid != NULL) {
        (*tid) = to_del->tid;
    }
    if (data != NULL) {
        (*data) = to_del->data;
    }
    (*des) = to_del->next;
    free(to_del);
    if ((*des) == NULL) {
        (*tail) = NULL;
    }
}

/* Check if the linked list is empty.
 */
static int linked_list_is_empty(linked_list_t *list) {
    return (list == NULL);
}


/* Mutex handling.
 */

typedef struct {
    char locked;                    // locked flag
    linked_list_t *wait_list;       // linked list for threads waiting for this lock
    linked_list_t *wait_list_tail;  // record the tail of the linked list
} mutex_t;

/* Initialize a mutex.
 */
int uthread_mutex_init(uthread_mutex_t *mutex) {
    mutex_t *p;

    in_critical = 1;

    p = (mutex_t *) mutex;
    p->locked = 0;
    p->wait_list = NULL;
    p->wait_list_tail = NULL;

    in_critical = 0;
    return 0;
}

/* Try locking a mutex.
 */
int uthread_mutex_trylock(uthread_mutex_t *mutex) {
    mutex_t *p;

    in_critical = 1;

    p = (mutex_t *) mutex;
    if (!p->locked) {
        p->locked = 1;

        in_critical = 0;
        return 0;
    } else {
        return -1;
    }
}

/* Lock a mutex.
 */
int uthread_mutex_lock(uthread_mutex_t *mutex) {
    mutex_t *p;

    in_critical = 1;
    
    p = (mutex_t *) mutex;
    if (!p->locked) {
        p->locked = 1;
        
        in_critical = 0;
        return 0;
    } else {
        if (is_ready_queue_empty()) {
            in_critical = 0;
            return EDEADLK;
        } else {
            uthread_t previous_thread_id;

            // put the current thread to the wait list
            linked_list_put_tail(&p->wait_list, &p->wait_list_tail, current_thread_id, NULL);
            previous_thread_id = current_thread_id;
            current_thread_id = get_ready_tid();

            in_critical = 0;
            swapcontext(&tcbs[previous_thread_id].uc, &tcbs[current_thread_id].uc);
            return 0;
        }
    }
}

/* Unlock a mutex.
 */
int uthread_mutex_unlock(uthread_mutex_t *mutex) {
    mutex_t *p;

    in_critical = 1;

    p = (mutex_t *) mutex;
    // if the wait list is empty, set the locked to 0
    // otherwise wake up a thread
    if (linked_list_is_empty(p->wait_list)) {
        p->locked = 0;

        in_critical = 0;
        return 0;
    } else {
        uthread_t next_thread_id;
        
        linked_list_get_head(&p->wait_list, &p->wait_list_tail, &next_thread_id, NULL);
        put_ready_tid(next_thread_id);

        in_critical = 0;
        return 0;
    }
}


/* Conditional variable handling.
 */

typedef struct {
    linked_list_t *wait_list;       // linked list for threads waiting for this conditional variable  
    linked_list_t *wait_list_tail;  // record the tail of the linked list
} cond_t;

/* Initialize a conditional variable.
 */
int uthread_cond_init(uthread_cond_t *cond) {
    cond_t *p;

    in_critical = 1;

    p = (cond_t *) cond;
    p->wait_list = NULL;
    p->wait_list_tail = NULL;

    in_critical = 0;
    return 0;
}

/* Wake up one thread waiting for condition variable COND.
 */
int uthread_cond_signal(uthread_cond_t *cond) {
    cond_t *p;

    in_critical = 1;

    p = (cond_t *) cond;
    if (!linked_list_is_empty(p->wait_list)) {
        uthread_t next_thread_id;
        mutex_t *mutex;

        linked_list_get_head(&p->wait_list, &p->wait_list_tail, &next_thread_id, (void **) &mutex);
        // put the thread from wait list of conditional variable to the
        // wait list of mutex
        linked_list_put_tail(&mutex->wait_list, &mutex->wait_list_tail, next_thread_id, NULL);
    }

    in_critical = 0;
    return 0;
}

/* Wake up all threads waiting for condition variables COND.
 */
int uthread_cond_broadcast(uthread_cond_t *cond) {
    cond_t *p;

    in_critical = 1;

    p = (cond_t *) cond;
    while (!linked_list_is_empty(p->wait_list)) {
        uthread_t next_thread_id;
        mutex_t *mutex;

        linked_list_get_head(&p->wait_list, &p->wait_list_tail, &next_thread_id, (void **) &mutex);
        linked_list_put_tail(&mutex->wait_list, &mutex->wait_list_tail, next_thread_id, NULL);
    }

    in_critical = 0;
    return 0;
}

/* Wait for condition variable COND to be signaled or broadcast.
 * MUTEX is assumed to be locked before.
 */
int uthread_cond_wait(uthread_cond_t *cond, uthread_mutex_t *mutex) {
    cond_t *p;
    
    in_critical = 1;

    p = (cond_t *) cond;
    linked_list_put_tail(&p->wait_list, &p->wait_list_tail, current_thread_id, (void *) mutex);

    // unlock the mutex and wait
    uthread_mutex_unlock(mutex);

    if (is_ready_queue_empty()) {
        exit(EDEADLK);
    } else {
        uthread_t previous_thread_id;

        previous_thread_id = current_thread_id;
        current_thread_id = get_ready_tid();

        in_critical = 0;
        swapcontext(&tcbs[previous_thread_id].uc, &tcbs[current_thread_id].uc);
        return 0;
    }
}
