//
// Created by Wayne on 16/1/13.
//

#ifndef UTHREAD_H
#define UTHREAD_H


#define UTHREAD_CANCELED ((void *) -1)

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
} uthread_mutex_t;

int uthread_mutex_init(uthread_mutex_t *mutex);
int uthread_mutex_destroy(uthread_mutex_t *mutex);
int uthread_mutex_trylock(uthread_mutex_t *mutex);
int uthread_mutex_lock(uthread_mutex_t *mutex);
int uthread_mutex_unlock(uthread_mutex_t *mutext);


typedef struct {
} uthread_rwlock_t;

int uthread_rwlock_init(uthread_rwlock_t *rwlock);
int uthread_rwlock_destroy(uthread_rwlock_t *rwlock);
int uthread_rwlock_rdlock(uthread_rwlock_t *rwlock);
int uthread_rwlock_tryrdlock(uthread_rwlock_t *rwlock);
int uthread_rwlock_wrlock(uthread_rwlock_t *rwlock);
int uthread_rwlock_trywrlock(uthread_rwlock_t *rwlock);
int uthread_rwlock_unlock(uthread_rwlock_t *rwlock);


typedef struct {
} uthread_cond_t;

int uthread_cond_init(uthread_cond_t *cond);
int utherad_cond_destroy(uthread_cond_t *cond);
int uthread_cond_signal(uthread_cond_t *cond);
int uthread_cond_broadcast(uthread_cond_t *cond);
int uthread_cond_wait(uthread_cond_t *cond, uthread_mutex_t *mutex);


typedef struct {
} uthread_spinlock_t;

int uthread_spin_init(uthread_spinlock_t *lock);
int uthread_spin_destroy(uthread_spinlock_t *lock);
int uthread_spin_lock(uthread_spinlock_t *lock);
int uthread_spin_trylock(uthread_spinlock_t *lock);
int uthread_spin_unlock(uthread_spinlock_t *lock);


typedef struct {
} uthread_barrier_t;

int uthread_barrier_init(uthread_barrier_t *barrier);
int uthread_barrier_destroy(uthread_barrier_t *barrier);
int uthread_barrier_wait(uthread_barrier_t *barrier);

#endif
