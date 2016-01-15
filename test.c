#include <stdio.h>
#include "uthread.h"

long fac(long n) {
    return (n == 0 ? 1 : n * fac(n - 1));
}

void *thread(void *arg) {
    uthread_detach(uthread_self());
    printf("I am thread %lu\n", uthread_self());
    int a = rand() % 100000000;
    while (a --);
    printf("fac(%ld) = %ld\n", (long) arg, fac((long) arg));
    return NULL;
}

void *foo(void *arg) {
    while (1) {
        printf("foo\n");
        uthread_yield();
    }
}

void *bar(void *arg) {
    while (1) {
        uthread_yield();
        printf("bar\n");
    }
}

int main(int argc, char *argv[]) {
    long i;
    uthread_t tid[10];

    srand(time(NULL));

    uthread_create(NULL, foo, NULL);
    uthread_create(NULL, bar, NULL);

    while (1);

    printf("Hello world!\n");

    return 0;
}
