//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include "uthread.h"

void* thread(void *arg) {
    long tmp;

    tmp = (long) arg;
    tmp += 1;

    uthread_exit((void *) tmp);
}

int main(int argc, char *argv[]) {
    uthread_t tids[1023];
    long i, sum;

    for (i = 0; i < 1023; ++ i) {
        uthread_create(&tids[i], thread, (void *) i);
    }

    sum = 0;
    for (i = 0; i < 1023; ++ i) {
        long tmp;

        uthread_join(tids[i], (void **) &tmp);
        sum += tmp;
    }

    printf("%ld\n", sum);

    return 0;
}
