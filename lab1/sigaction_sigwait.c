#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>

sigset_t mask;
int use_sigaction = 1;  // Прапорець для переключення режимів

void sig_handler(int sig) {
    printf("Handler: Thread %ld received signal %d (via sigaction)\n",
           (long)syscall(SYS_gettid), sig);
}

void* thread_sigwait(void* arg) {
    // Додаємо явне блокування SIGUSR1 для цього потоку
    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &block_mask, NULL);
    int sig;

    printf("Thread (sigwait) %ld waiting...\n",
           (long)syscall(SYS_gettid));
    if (sigwait(&mask, &sig) != 0) {
        perror("sigwait");
        return NULL;
    }
    printf("Thread (sigwait) %ld got signal %d\n",
           (long)syscall(SYS_gettid), sig);
    return NULL;
}

void* thread_sigaction(void* arg) {
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Виправлено: знімаємо блокування SIGUSR1 для цього потоку у режимі priority=1
    if (use_sigaction) {
        sigset_t unblock_mask;
        sigemptyset(&unblock_mask);
        sigaddset(&unblock_mask, SIGUSR1);
        pthread_sigmask(SIG_UNBLOCK, &unblock_mask, NULL);
    }

    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        perror("sigaction");
        return NULL;
    }
    
    printf("Thread (sigaction) %ld set handler (signal %s)\n",
           (long)syscall(SYS_gettid),
           use_sigaction ? "unblocked" : "blocked");
    while (1) {
        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        use_sigaction = atoi(argv[1]);
    }
    
    pthread_t t1, t2;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    
    printf("\n=== Mode: %s ===\n",
         use_sigaction ? "sigaction priority" : "sigwait priority");
    
    // Спочатку створюємо потік sigaction
    if (pthread_create(&t2, NULL, thread_sigaction, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    sleep(1);  // Даємо час на ініціалізацію

    // Потім створюємо потік sigwait
    if (pthread_create(&t1, NULL, thread_sigwait, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
        
    sleep(1);
    printf("Sending SIGUSR1 to process %d...\n", getpid());
    kill(getpid(), SIGUSR1);
    
    sleep(1);  // Даємо час на обробку сигналу
    pthread_cancel(t2);  // Завершуємо нескінченний цикл
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    return 0;
}
