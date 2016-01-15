#include <stdio.h>
#include "uthread.h"

long cnt = 0;
uthread_mutex_t mut;

void *thread(void *arg) {
    int n = 100000;
    int i;

    for (i = 0; i < n; ++ i) {
        uthread_mutex_lock(&mut);
        cnt ++;
        uthread_mutex_unlock(&mut);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    uthread_t tids[10];
    int i;

    uthread_mutex_init(&mut);

    for (i = 0; i < 10; ++ i) {
        uthread_create(&tids[i], thread, NULL);
    }

    for (i = 0; i < 10; ++ i) {
        uthread_join(tids[i], NULL);
    }

    printf("%ld\n", cnt);

    return 0;
}
