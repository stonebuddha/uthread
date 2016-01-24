//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include "uthread.h"

static long cnt = 0;
static int have = 0;

uthread_mutex_t mut;
uthread_cond_t c1, c2;

long buf;

void* thread1(void *arg) {
    long i;

    uthread_mutex_lock(&mut);

    for (i = 0; i < 1000; ++ i) {
        while (!have) {
            uthread_cond_wait(&c1, &mut);
        }
        cnt += buf;
        have = 0;
        uthread_cond_signal(&c2);
    }

    uthread_mutex_unlock(&mut);

    return NULL;
}

void* thread2(void *arg) {
    long t = (long) arg;
    long i;

    uthread_mutex_lock(&mut);

    for (i = t; i < t + 1000; ++ i) {
        while (have) {
            uthread_cond_wait(&c2, &mut);
        }
        buf = i;
        have = 1;
        uthread_cond_signal(&c1);
    }
    
    uthread_mutex_unlock(&mut);

    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t a[2];
    uthread_t b[2];

    uthread_mutex_init(&mut);
    uthread_cond_init(&c1);
    uthread_cond_init(&c2);

    uthread_create(&a[0], thread1, NULL);
    uthread_create(&a[1], thread1, NULL);
    uthread_create(&b[0], thread2, (void *) 0);
    uthread_create(&b[1], thread2, (void *) 1000);

    uthread_join(a[0], NULL);
    uthread_join(a[1], NULL);
    uthread_join(b[0], NULL);
    uthread_join(b[1], NULL);

    printf("%ld\n", cnt);

    return 0;
}
