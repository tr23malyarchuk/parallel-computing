#define _GNU_SOURCE // for SYS_gettid
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>  // for syscall and SYS_gettid
#include <errno.h>
#include <string.h>

#define THREAD_COUNT 5

int signal_count[THREAD_COUNT] = {0};

// Оброблювач сигналу SIGTERM
void sigterm_handler()
{
    for (int i = 0; i < THREAD_COUNT; i++)
    printf("%d-й потік отримав %d сигналів\n", i, signal_count[i]);
    exit(EXIT_SUCCESS);
}

// Початкова функція потоку
void* thread_func(void* arg) {
    sigset_t mask;
    int sig;
    int thread_num = *(int*)arg;
    int err;

    // Ініціалізція множини сигналів mask як порожньої множини
    if(sigemptyset(&mask) != 0)
    {
        fprintf(stderr, "Помилка виклику функції sigemptyset: %s\n", strerror(errno));
        pthread_exit(NULL);
    }

    // Додаємо сигнал SIGUSR1 до множини сигналів mask 
    if(sigaddset(&mask, SIGUSR1) != 0)
    {
        fprintf(stderr, "Помилка виклику функції sigaddset: %s\n", strerror(errno));
        pthread_exit(NULL);
    }

    // Додаємо сигнали з множини сигналів mask до поточної маски сигналів потоку
    if((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
    {
        fprintf(stderr, "Помилка виклику функції pthread_sigmask: %s\n", strerror(err));
        pthread_exit(NULL);
    }

    printf("Thread %d (TID=%ld) waiting for signal...\n", 
           thread_num, (long)syscall(SYS_gettid));
    
    // Очікуємо надходження сигналів, зазначених у множині сигналів mask
    if ((err = sigwait(&mask, &sig)) != 0) {
        fprintf(stderr, "Помилка виклику функції sigwait: %s\n", strerror(err));
        pthread_exit(NULL);
    }
    else {
        printf("Thread %d received signal %d\n", thread_num, sig); 
        // нарощуємо значення лічильника прийнятих сигналів відповідним потоком
        signal_count[thread_num]++;
    }
    
    return NULL;
}

int main() {
    sigset_t mask;
    pthread_t threads[THREAD_COUNT], thread_num;
    int err;
    struct sigaction sa;
    
    if(sigemptyset(&mask) != 0)
    {
        fprintf(stderr, "Помилка виклику функції sigemptyset: %s\n", strerror(errno));
        pthread_exit(NULL);
    }
    if(sigaddset(&mask, SIGUSR1) != 0)
    {
        fprintf(stderr, "Помилка виклику функції sigaddset: %s\n", strerror(errno));
        pthread_exit(NULL);
    }
    if((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
    {
        fprintf(stderr, "Помилка виклику функції pthread_sigmask: %s\n", strerror(err));
        pthread_exit(NULL);
    }
    
    sa.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sa, NULL);

    printf("Процес номер %d запущений\n", getpid());

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_create(&thread_num, NULL, thread_func, (void*)i) != 0)
        {
            perror("pthread_create");
            exit(1);
        }
    }
    while(1);
    return 0;
}
