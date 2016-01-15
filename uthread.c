//
// Created by Wayne on 16/1/13.
//

#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include "uthread.h"

#define STACK_SIZE 4096
#define POOL_SIZE 1024
#define TIME_SLICE 10000

typedef struct {
    ucontext_t uc;
    void *retval;
    uthread_t joined;
    void **joined_retval;
    int valid;
    int exited;
    int detached;
    int canceled;
    char stack[STACK_SIZE];
} thread_control_block;

static thread_control_block tcbs[POOL_SIZE];
static int inited = 0;
static uthread_t current_thread_id;
static struct itimerval time_slice;
static struct sigaction scheduler;
static int in_critical = 0;


static uthread_t free_tids[POOL_SIZE + 1];
static int free_tids_head = 0, free_tids_tail = 0;

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

static void put_fresh_tid(uthread_t tid) {
    free_tids[free_tids_tail] = tid;
    free_tids_tail = (free_tids_tail + 1) % (POOL_SIZE + 1);
}

static int is_fresh_queue_empty() {
    return (free_tids_head == free_tids_tail);
}


static uthread_t ready_queue[POOL_SIZE + 1];
static int ready_queue_head = 0, ready_queue_tail = 0;

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

static void put_ready_tid(uthread_t tid) {
    ready_queue[ready_queue_tail] = tid;
    ready_queue_tail = (ready_queue_tail + 1) % (POOL_SIZE + 1);
}

static int is_ready_queue_empty() {
    return (ready_queue_head == ready_queue_tail);
}


static void schedule(int sig) {
    if (in_critical || is_ready_queue_empty()) {
        return;
    } else {
        uthread_t previous_thread_id;
        int flag = 0;

        put_ready_tid(current_thread_id);
        previous_thread_id = current_thread_id;
        while (!is_ready_queue_empty()) {
            current_thread_id = get_ready_tid();
            
            if (!tcbs[current_thread_id].canceled) {
                flag = 1;
                break;
            } else {
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

static void thread_stub(void *(*start_routine) (void *), void *arg) {
    uthread_exit(start_routine(arg));
}

static int uthread_init() {
    uthread_t tid;
    uthread_t main_thread_id;
    thread_control_block *tcb;
    int tmp;

    for (tid = 0; tid < POOL_SIZE; ++ tid) {
        tcbs[tid].valid = 0;
        put_fresh_tid(tid);
    }

    scheduler.sa_handler = schedule;
    scheduler.sa_flags = SA_RESTART;
    sigemptyset(&scheduler.sa_mask);
    sigaddset(&scheduler.sa_mask, SIGPROF);
    if ((tmp = sigaction(SIGPROF, &scheduler, NULL)) != 0) {
        return tmp;
    }

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

    time_slice.it_value.tv_sec = 0;
    time_slice.it_value.tv_usec = TIME_SLICE;
    time_slice.it_interval = time_slice.it_value;
    if ((tmp = setitimer(ITIMER_PROF, &time_slice, NULL)) != 0) {
        return tmp;
    }

    inited = 1;

    return 0;
}

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

    if (is_fresh_queue_empty()) {
        in_critical = 0;
        return EAGAIN;
    }
    tid = get_fresh_tid();
    if (thread != NULL) {
        (*thread) = tid;
    }

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

void uthread_exit(void *retval) {
    in_critical = 1;

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

uthread_t uthread_self() {
    return current_thread_id;
}

int uthread_equal(uthread_t t1, uthread_t t2) {
    return (t1 == t2);
}

void uthread_yield() {
    schedule(0);
}

int uthread_cancel(uthread_t thread) {
    in_critical = 1;

    if (thread >= POOL_SIZE || !tcbs[thread].valid) {
        in_critical = 0;
        return ESRCH;
    } else {
        tcbs[thread].canceled = 1;
        
        in_critical = 0;
        return 0;
    }
}


typedef struct _linked_list_t {
    uthread_t tid;
    struct _linked_list_t *next;
} linked_list_t;

static void linked_list_put_tail(linked_list_t **des, uthread_t tid) {
    while ((*des) != NULL) {
        des = &((*des)->next);
    }
    (*des) = (linked_list_t *) malloc(sizeof(linked_list_t));
    (*des)->tid = tid;
    (*des)->next = NULL;
}

static uthread_t linked_list_get_head(linked_list_t **des) {
    linked_list_t *to_del;
    uthread_t res;

    to_del = (*des);
    res = to_del->tid;
    (*des) = to_del->next;
    free(to_del);

    return res;
}

static int linked_list_is_empty(linked_list_t *list) {
    return (list == NULL);
}


typedef struct {
    int locked;
    linked_list_t *wait_list;
} _uthread_mutex_t;

int uthread_mutex_init(uthread_mutex_t *mutex) {
    _uthread_mutex_t *p;

    in_critical = 1;

    p = (_uthread_mutex_t *) mutex;
    p->locked = 0;
    p->wait_list = NULL;

    in_critical = 0;
    return 0;
}

int uthread_mutex_trylock(uthread_mutex_t *mutex) {
    _uthread_mutex_t *p;

    in_critical = 1;

    p = (_uthread_mutex_t *) mutex;
    if (!p->locked) {
        p->locked = 1;

        in_critical = 0;
        return 0;
    } else {
        return -1;
    }
}

int uthread_mutex_lock(uthread_mutex_t *mutex) {
    _uthread_mutex_t *p;

    in_critical = 1;
    
    p = (_uthread_mutex_t *) mutex;
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

            linked_list_put_tail(&p->wait_list, current_thread_id);
            previous_thread_id = current_thread_id;
            current_thread_id = get_ready_tid();

            in_critical = 0;
            swapcontext(&tcbs[previous_thread_id].uc, &tcbs[current_thread_id].uc);
            return 0;
        }
    }
}

int uthread_mutex_unlock(uthread_mutex_t *mutex) {
    _uthread_mutex_t *p;

    in_critical = 1;

    if (linked_list_is_empty(p->wait_list)) {
        p->locked = 0;

        in_critical = 0;
        return 0;
    } else {
        uthread_t next_thread_id;
        
        next_thread_id = linked_list_get_head(&p->wait_list);
        put_ready_tid(next_thread_id);

        in_critical = 0;
        return 0;
    }
}
