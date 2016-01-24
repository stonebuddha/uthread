//
// Created by Wayne on 24/1/13.
//

#include <stdio.h>
#include "uthread.h"

void* thread(void *arg) {
    while (1);
}

int main(int argc, char *argv[]) {
    uthread_t a, b;

    uthread_create(&a, thread, NULL);
    uthread_create(&b, thread, NULL);

    printf("%d%d%d%d\n", uthread_equal(a, a), uthread_equal(a, b), uthread_equal(b, a), uthread_equal(b, b));

    return 0;
}
