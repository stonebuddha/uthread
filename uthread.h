//
// Created by Wayne on 16/1/13.
//

#ifndef UTHREAD_H
#define UTHREAD_H


#define UTHREAD_CANCELED ((void *) -1)

#define UTHREAD_MUTEX_SIZE 16
#define UTHREAD_COND_SIZE 8

typedef unsigned long int uthread_t;

int uthread_create(uthread_t *thread, void *(*start_routine) (void *), void *arg);
void uthread_exit(void *retval);
int uthread_join(uthread_t thread, void **retval);
int uthread_detach(uthread_t thread);
uthread_t uthread_self(void);
int uthread_equal(uthread_t t1, uthread_t t2);
void uthread_yield(void);
int uthread_cancel(uthread_t thread);


typedef struct {
    char span[UTHREAD_MUTEX_SIZE];
} uthread_mutex_t;

int uthread_mutex_init(uthread_mutex_t *mutex);
int uthread_mutex_trylock(uthread_mutex_t *mutex);
int uthread_mutex_lock(uthread_mutex_t *mutex);
int uthread_mutex_unlock(uthread_mutex_t *mutext);


typedef struct {
    char span[UTHREAD_COND_SIZE];
} uthread_cond_t;

int uthread_cond_init(uthread_cond_t *cond);
int uthread_cond_signal(uthread_cond_t *cond);
int uthread_cond_broadcast(uthread_cond_t *cond);
int uthread_cond_wait(uthread_cond_t *cond, uthread_mutex_t *mutex);

#endif
