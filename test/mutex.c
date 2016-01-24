//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include "uthread.h"

static long cnt = 0;

uthread_mutex_t mut;

void* thread1(void *arg) {
    int i;

    uthread_mutex_lock(&mut);

    for (i = 0; i < 100000000; ++ i) {
        ++ cnt;
    }

    uthread_mutex_unlock(&mut);

    return NULL;
}

void* thread2(void *arg) {
    int i;

    uthread_mutex_lock(&mut);

    for (i = 0; i < 10; ++ i) {
        cnt *= 2;
    }

    uthread_mutex_unlock(&mut);

    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t a;
    uthread_t b;

    uthread_mutex_init(&mut);

    uthread_create(&a, thread1, NULL);
    uthread_create(&b, thread2, NULL);

    uthread_join(a, NULL);
    uthread_join(b, NULL);

    printf("%ld\n", cnt);

    return 0;
}
