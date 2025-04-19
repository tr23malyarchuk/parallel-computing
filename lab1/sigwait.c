#define _GNU_SOURCE // for SYS_gettid
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>  // for syscall and SYS_gettid

sigset_t mask;

void* thread_func(void* arg) {
    int sig;
    int thread_num = *(int*)arg;
    printf("Thread %d (TID=%ld) waiting for signal...\n", 
           thread_num, (long)syscall(SYS_gettid));
    
    if (sigwait(&mask, &sig) != 0) {
        perror("sigwait");
        pthread_exit(NULL);
    }
    
    printf("Thread %d received signal %d\n", thread_num, sig);
    return NULL;
}

int main() {
    pthread_t threads[3];
    int thread_nums[3] = {1, 2, 3};
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    
    for (int i = 0; i < 3; i++) {
        if (pthread_create(&threads[i],
             NULL, thread_func, &thread_nums[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    sleep(1);
    printf("Sending SIGUSR1 to process %d...\n", getpid());
    kill(getpid(), SIGUSR1);
    
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
