#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#define THREADS 10

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t bath_is_full;
    pthread_cond_t bath_is_empty;
    int girls, bois;
} MutexStructure;


void entering(int, MutexStructure*);
void exiting(int, MutexStructure*);
void* thread_process_enter(void*);
void* thread_process_exit(void*);

int main() {
    MutexStructure* mutex = malloc(sizeof(MutexStructure));
    pthread_t threads[THREADS];
    int return_code;

    mutex->girls = 0;
    mutex->bois = 0;
    srand(time(NULL));   // Initialization, should only be called once.
    pthread_mutex_init(&mutex->mutex, NULL);
    pthread_cond_init(&mutex->bath_is_full, NULL);
    pthread_cond_init(&mutex->bath_is_empty, NULL);

    for(long t = 0; t < THREADS / 2; t++) {
        return_code = pthread_create(&threads[2 * t], NULL, thread_process_enter, mutex);
        if(return_code) {
            printf("Exiting beccause something went wrong creating a thread");
            exit(-1);
        }
        return_code = pthread_create(&threads[2 * t + 1], NULL, thread_process_exit, mutex);
        if(return_code) {
            printf("Exiting beccause something went wrong creating a thread");
            exit(-1);
        }
    }

    for(int t = 0; t < THREADS; t++) {
        pthread_join(threads[t], NULL);
    }
    free(mutex);
    return 0;
}

void entering(int is_girl, MutexStructure* mutex) {
    int r = rand() % 500;
    if(is_girl) {
        printf("A girl wants to enter the bathroom.\n");
    } else {
        printf("A boi wants to enter the bathroom.\n");
    }
    pthread_mutex_lock(&mutex->mutex);
    while(mutex->girls == 1 || mutex->bois == 5) {
        printf("A %d is waiting to enter\n", is_girl);
        pthread_cond_wait(&mutex->bath_is_full, &mutex->mutex);
    }
    (is_girl)? mutex->girls++ : mutex->bois++;
    pthread_cond_signal(&mutex->bath_is_empty);
    printf("A %d is entering\n", is_girl);
    pthread_mutex_unlock(&mutex->mutex);
    usleep(r * 1000);
}

void exiting(int is_girl, MutexStructure* mutex) {
    int r = rand() % 500;
    if(is_girl) {
        printf("A girl wants to exit the bathroom.\n");
    } else {
        printf("A boi wants to exit the bathroom.\n");
    }
    pthread_mutex_lock(&mutex->mutex);
    while(mutex->girls == 0 || mutex->bois == 0) {
        printf("A %d is waiting to exit\n", is_girl);
        pthread_cond_wait(&mutex->bath_is_empty, &mutex->mutex);
    }
    (is_girl)? mutex->girls-- : mutex->bois--;
    pthread_cond_signal(&mutex->bath_is_full);
    printf("A %d is exiting\n", is_girl);
    pthread_mutex_unlock(&mutex->mutex);
    usleep(r * 1000);
}

void* thread_process_enter(void* mutex_void) {
    MutexStructure* mutex = (MutexStructure*) mutex_void;
    int t = 0;
    while(1) {
        entering(t % 2, mutex);
        t++;
    }
} 

void* thread_process_exit(void* mutex_void) {
    MutexStructure* mutex = (MutexStructure*) mutex_void;
    int t = 0;
    while(1) {
        exiting(t % 2, mutex);
        t++;
    }
} 