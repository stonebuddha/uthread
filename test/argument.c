//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include "uthread.h"

static long sum = 0;

void* thread(void *arg) {
    sum += (long) arg;

    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t tids[1023];
    long i;

    for (i = 0; i < 1023; ++ i) {
        uthread_create(&tids[i], thread, (void *) i);
    }

    for (i = 0; i < 1023; ++ i) {
        uthread_join(tids[i], NULL);
    }

    printf("%ld\n", sum);

    return 0;
}
