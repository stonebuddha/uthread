//
// Created by Wayne on 16/1/13.
//

#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include "uthread.h"

typedef struct {
    ucontext_t uc;
    void *retval;
    char stack[UTHREAD_STACK_SIZE];
    uthread_t joining;
    void **joining_status;
    int valid;
    int exited;
} thread_control_block;

static thread_control_block tcbs[UTHREAD_POOL_SIZE];
static uthread_t free_tids[UTHREAD_POOL_SIZE + 1];
static int free_tids_head = 0, free_tids_tail = 0;

static struct itimerval time_slice;
static struct sigaction scheduler;

static int inited = 0;
static uthread_t current_thread_id;
static uthread_t ready_queue[UTHREAD_POOL_SIZE + 1];
static int ready_queue_head = 0, ready_queue_tail = 0;

static void schedule(int sig) {
    if (ready_queue_head == ready_queue_tail) {
        return;
    } else {
        uthread_t previous_thread_id;

        ready_queue[ready_queue_tail] = current_thread_id;
        ready_queue_tail = (ready_queue_tail + 1) % (UTHREAD_POOL_SIZE + 1);
        previous_thread_id = current_thread_id;
        current_thread_id = ready_queue[ready_queue_head];
        ready_queue_head = (ready_queue_head + 1) % (UTHREAD_POOL_SIZE + 1);

        swapcontext(&tcbs[previous_thread_id].uc, &tcbs[current_thread_id].uc);
    }
}

static void stub(void *(*start_routine) (void *), void *arg) {
    uthread_exit(start_routine(arg));
}

static uthread_t get_fresh_tid() {
    if (free_tids_head == free_tids_tail) {
        return UTHREAD_POOL_SIZE;
    } else {
        uthread_t res;

        res = free_tids[free_tids_head];
        free_tids_head = (free_tids_head + 1) % (UTHREAD_POOL_SIZE + 1);

        return res;
    }
}

static void put_fresh_tid(uthread_t tid) {
    free_tids[free_tids_tail] = tid;
    free_tids_tail = (free_tids_tail + 1) % (UTHREAD_POOL_SIZE + 1);
}

static int uthread_init() {
    uthread_t tid;
    uthread_t main_thread_id;
    thread_control_block *tcb;

    for (tid = 0; tid < UTHREAD_POOL_SIZE; ++ tid) {
        tcbs[tid].valid = 0;
        put_fresh_tid(tid);
    }

    scheduler.sa_handler = schedule;
    scheduler.sa_flags = SA_RESTART;
    sigemptyset(&scheduler.sa_mask);
    sigaddset(&scheduler.sa_mask, SIGPROF);
    if (sigaction(SIGPROF, &scheduler, NULL) != 0) {
        return -1;
    }

    main_thread_id = get_fresh_tid();
    if (main_thread_id == UTHREAD_POOL_SIZE) {
        return -1;
    }
    tcb = &tcbs[main_thread_id];
    getcontext(&tcb->uc);
    tcb->joining = UTHREAD_POOL_SIZE;
    tcb->valid = 1;
    tcb->exited = 0;
    current_thread_id = main_thread_id;

    time_slice.it_value.tv_sec = 0;
    time_slice.it_value.tv_usec = UTHREAD_TIME_SLICE;
    time_slice.it_interval = time_slice.it_value;
    if (setitimer(ITIMER_PROF, &time_slice, NULL) != 0) {
        return -1;
    }

    inited = 1;

    return 0;
}

int uthread_create(uthread_t *thread, void *(*start_routine) (void *), void *arg) {
    thread_control_block *tcb;
    uthread_t tid;

    if (!inited) {
        int init_res = uthread_init();
        if (init_res != 0) {
            return init_res;
        }
    }

    tid = get_fresh_tid();
    if (tid == UTHREAD_POOL_SIZE) {
        return -1;
    }
    (*thread) = tid;

    tcb = &tcbs[tid];
    getcontext(&tcb->uc);
    tcb->uc.uc_stack.ss_sp = (void*) tcb->stack;
    tcb->uc.uc_stack.ss_size = UTHREAD_STACK_SIZE;
    tcb->joining = UTHREAD_POOL_SIZE;
    tcb->valid = 1;
    tcb->exited = 0;
    makecontext(&tcb->uc, (void (*) (void)) stub, 2, start_routine, arg);

    ready_queue[ready_queue_tail] = tid;
    ready_queue_tail = (ready_queue_tail + 1) % (UTHREAD_POOL_SIZE + 1);

    return 0;
}

void uthread_exit(void *retval) {
    uthread_t tid;

    tcbs[current_thread_id].retval = retval;
    tcbs[current_thread_id].exited = 1;

    for (tid = 0; tid < UTHREAD_POOL_SIZE; ++ tid) {
        if (tcbs[tid].valid && !tcbs[tid].exited && tcbs[tid].joining == current_thread_id) {
            tcbs[tid].joining = UTHREAD_POOL_SIZE;
            if (tcbs[tid].joining_status != NULL) {
                (*tcbs[tid].joining_status) = retval;
            }
            ready_queue[ready_queue_tail] = tid;
            ready_queue_tail = (ready_queue_tail + 1) % (UTHREAD_POOL_SIZE + 1);
        }
    }

    if (ready_queue_head == ready_queue_tail) {
        exit(-1);
    } else {
        current_thread_id = ready_queue[ready_queue_head];
        ready_queue_head = (ready_queue_head + 1) % (UTHREAD_POOL_SIZE + 1);
        setcontext(&tcbs[current_thread_id].uc);
    }
}

int uthread_join(uthread_t thread, void **status) {
    if (tcbs[thread].valid) {
        if (!tcbs[thread].exited) {
            tcbs[current_thread_id].joining = thread;
            tcbs[current_thread_id].joining_status = status;

            if (ready_queue_head == ready_queue_tail) {
                exit(-1);
            } else {
                uthread_t previous_thread_id;

                previous_thread_id = current_thread_id;
                current_thread_id = ready_queue[ready_queue_head];
                ready_queue_head = (ready_queue_head + 1) % (UTHREAD_POOL_SIZE + 1);

                swapcontext(&tcbs[previous_thread_id].uc, &tcbs[current_thread_id].uc);
            }

            return 0;
        } else {
            if (status != NULL) {
                (*status) = tcbs[thread].retval;
            }
            return 0;
        }
    } else {
        return -1;
    }
}

uthread_t uthread_self() {
    return current_thread_id;
}
