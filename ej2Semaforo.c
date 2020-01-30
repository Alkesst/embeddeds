#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#define THREADS 2

sem_t semaphore;
unsigned long shared_resource;

void* sumar(void*);
void* lectura(void*);

int main() {
    pthread_t threads_modificadores[THREADS];
    pthread_t threads_lectura[THREADS];
    int id_thread_modificador[THREADS];
    int id_thread_lector[THREADS];
    int return_code; 
    shared_resource = 0;
    // semaphore with only 1 call.
    sem_init(&semaphore, 1, 1);

    for(long t = 0; t < THREADS; t++) {
        id_thread_modificador[t] = t;
        return_code = pthread_create(&threads_modificadores[t], NULL, sumar, &id_thread_modificador[t]);
        printf("\n");
        if(return_code) {
            printf("ERROR: Pthread_create returned an error code %d", return_code);
            exit(-1);
        }
    }

    for(long t = 0; t < THREADS; t++) {
        id_thread_lector[t] = t + 2;
        return_code = pthread_create(&threads_lectura[t], NULL, lectura, &id_thread_lector[t]);
        printf("\n");
        if(return_code) {
            printf("ERROR: Pthread_create returned an error code %d", return_code);
            exit(-1);
        }
    }

    for(int i = 0; i < THREADS; i++) {
        pthread_join(threads_modificadores[i], NULL);
        pthread_join(threads_lectura[i], NULL);
    }

    return 0;
}

void* sumar(void* id) {
    int k = 0;
    while(1) {
        sem_wait(&semaphore);
        printf("Thread with id %d is in the mutex zone and has increased the shared resource\n", *(int*) id);
        sleep(1);
        shared_resource += 100;
        sem_post(&semaphore);    
        for(int i = 0; i < 1000000; i++) {k = k+1; k = k - 1;}
    }
    return 0;
}

void* lectura(void* id) {
    int k = 0;
    while(1) {
        sem_wait(&semaphore);
        printf("Thread with id %d is in the mutex zone and has read the value %lu\n", *(int*) id, shared_resource);
        sleep(1);
        sem_post(&semaphore);
        for(int i = 0; i < 1000000; i++) {k = k+1; k = k - 1;}
    }
    return 0;
}