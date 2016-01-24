//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include "uthread.h"

static int cnt = 0;

void* thread1(void *arg) {
    uthread_detach(uthread_self());

    cnt += 1;

    return NULL;
}

void* thread2(void *arg) {
    cnt += 1;

    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t tids[1023];
    long i;

    for (i = 0; i < 1023; ++ i) {
        uthread_create(&tids[i], thread1, NULL);
    }

    for (i = 0; i < 100000000; ++ i);

    for (i = 0; i < 1023; ++ i) {
        uthread_create(&tids[i], thread2, NULL);
    }
    
    for (i = 0; i < 1023; ++ i) {
        uthread_join(tids[i], NULL);
    }

    printf("%d\n", cnt);

    return 0;
}
