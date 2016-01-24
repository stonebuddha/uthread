//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include "uthread.h"

static int cnt = 0;

void* thread(void *arg) {
    cnt += 1;

    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t tids[1024];
    long i;

    for (i = 0; i < 1024; ++ i) {
        uthread_create(&tids[i], thread, NULL);
    }

    for (i = 0; i < 1024; ++ i) {
        uthread_join(tids[i], NULL);
    }

    printf("%d\n", cnt);

    return 0;
}
