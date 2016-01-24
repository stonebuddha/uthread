//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "uthread.h"

#define N 100

uthread_mutex_t chops[N];

void* thread(void *arg) {
    long i, j, k;
    long res = 0;

    i = (long) arg;
    j = (i + 1) % N;
    if (i > j) {
        long t;

        t = i;
        i = j;
        j = t;
    }

    for (k = 0; k < 10; ++ k) {
        int thinking, eating;
        thinking = rand() % 1000000;
        while (thinking != 0) {
            thinking --;
        }
        uthread_mutex_lock(&chops[i]);
        uthread_mutex_lock(&chops[j]);
        eating = rand() % 1000000;
        while (eating != 0) {
            eating --;
        }        
        ++ res;
        uthread_mutex_unlock(&chops[j]);
        uthread_mutex_unlock(&chops[i]);
    }

    return (void *) res;
}

int main(int argc, char *argv[]) {
    uthread_t tids[N];
    long i;
    long sum = 0;

    srand(time(NULL));
    
    for (i = 0; i < N; ++ i) {
        uthread_mutex_init(&chops[i]);
    }

    for (i = 0; i < N; ++ i) {
        uthread_create(&tids[i], thread, (void *) i);
    }

    for (i = 0; i < N; ++ i) {
        long tmp;

        uthread_join(tids[i], (void **) &tmp);
        sum += tmp;
    }

    printf("%ld\n", sum);

    return 0;
}
