#include <stdio.h>
#include "uthread.h"

long fac(long n) {
    return (n == 0 ? 1 : n * fac(n - 1));
}

void *thread(void *arg) {
    printf("I am thread %lu\n", uthread_self());
    int a = rand() % 100000000;
    while (a --);
    printf("fac(%ld) = %ld\n", (long) arg, fac((long) arg));
    return NULL;
}

int main(int argc, char *argv[]) {
    long i;
    uthread_t tid[10];

    srand(time(NULL));

    for (i = 0; i < 10; i++) {
        uthread_create(&tid[i], thread, (void *) i);
    }

    for (i = 0; i < 10; i++) {
        uthread_join(tid[i], NULL);
    }

    printf("Hello world!\n");

    return 0;
}
