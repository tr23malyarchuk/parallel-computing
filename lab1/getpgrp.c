#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

void* thread_func(void* arg) {
    printf("Thread: PID=%d, TID=%ld, PGID=%d, PSTID=%lu \n", 
           getpid(), syscall(SYS_gettid), getpgrp(),
           pthread_self());
    return NULL;
}

int main() {
    printf("Main thread: PID=%d, TID=%ld, PGID=%d, PSTID=%lu\n", 
           getpid(), syscall(SYS_gettid), getpgrp(), pthread_self());

    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_join(thread, NULL);

    return 0;
}
