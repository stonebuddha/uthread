//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "uthread.h"

static long cnt = 0;

uthread_sem_t empty, full;
uthread_mutex_t mut;

int buf[5], qh = 0, qt = 0;

void* producer(void *arg) {
    int i, j;

    for (i = 0; i < 100; ++ i) {
        uthread_sem_down(&empty);
        uthread_mutex_lock(&mut);
        for (j = rand() % 1000000; j != 0; -- j);
        buf[qt] = i;
        qt = (qt + 1) % 5;
        uthread_mutex_unlock(&mut);
        uthread_sem_up(&full);
    }

    return NULL;
}

void* consumer(void *arg) {
    int i, j;

    for (i = 0; i < 100; ++ i) {
        uthread_sem_down(&full);
        uthread_mutex_lock(&mut);
        for (j = rand() % 1000000; j != 0; -- j);
        cnt += buf[qh];
        qh = (qh + 1) % 5;
        uthread_mutex_unlock(&mut);
        uthread_sem_up(&empty);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t a[2];
    uthread_t b[2];
    int i;

    srand(time(NULL));
    uthread_mutex_init(&mut);
    uthread_sem_init(&empty, 5);
    uthread_sem_init(&full, 0);

    for (i = 0; i < 2; ++ i) {
        uthread_create(&a[i], producer, NULL);
    }
    for (i = 0; i < 2; ++ i) {
        uthread_create(&b[i], consumer, NULL);
    }

    for (i = 0; i < 2; ++ i) {
        uthread_join(a[i], NULL);
    }
    for (i = 0; i < 2; ++ i) {
        uthread_join(b[i], NULL);
    }

    printf("%ld\n", cnt);

    return 0;
}
