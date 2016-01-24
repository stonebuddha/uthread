#include <stdio.h>
#include "uthread.h"

uthread_mutex_t mut;

void *thread(void *arg) {
    int i;

    for (i = 0; i < 1000; ++ i) {
        uthread_mutex_lock(&mut);
        printf("Hello world! %ld\n", (long) arg);
        uthread_mutex_unlock(&mut);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t tids[1023];
    long i;

    uthread_mutex_init(&mut);

    for (i = 0; i < 1023; ++ i) {
        uthread_create(&tids[i], thread, (void *) i);
    }

    for (i = 0; i < 1023; ++ i) {
        uthread_join(tids[i], NULL);
    }

    return 0;
}
