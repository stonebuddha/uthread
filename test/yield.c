//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include "uthread.h"

static long cnt = 0;

void* thread1(void *arg) {
    int i;

    for (i = 0; i < 10; ++ i) {
        cnt += 1;
        uthread_yield();
    }

    return NULL;
}

void* thread2(void *arg) {
    int i ;
    
    for (i = 0; i < 10; ++ i) {
        cnt *= 2;
        uthread_yield();
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t a;
    uthread_t b;

    uthread_create(&a, thread1, NULL);
    uthread_create(&b, thread2, NULL);

    uthread_join(a, NULL);
    uthread_join(b, NULL);

    printf("%ld\n", cnt);

    return 0;
}
