#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#define THREADS 2

unsigned long shared_resource;

pthread_mutex_t mutex;

void* sumar(void*);
void* lectura(void*);

int main() {
    pthread_t threads_modificadores[THREADS];
    pthread_t threads_lectura[THREADS];
    int id_thread_modificador[THREADS];
    int id_thread_lector[THREADS];
    int return_code; 
    shared_resource = 0;
    pthread_mutex_init(&mutex, NULL);

    for(int t = 0; t < THREADS; t++) {
        id_thread_lector[t] = 2 * t;
        id_thread_modificador[t] = 2 * t + 1;
        return_code = pthread_create(&threads_modificadores[t], NULL, sumar, &id_thread_modificador[t]);
        if(return_code) {
            printf("ERROR pthread_create returned %d\n", return_code);
            exit(-1);
        }
        return_code = pthread_create(&threads_lectura[t], NULL, lectura, &id_thread_lector[t]);
        if(return_code) {
            printf("ERROR pthread_create returned %d\n", return_code);
            exit(-1);
        }
    }

    for(int t = 0; t < THREADS; t++) {
        pthread_join(threads_lectura[t], NULL);
        pthread_join(threads_modificadores[t], NULL);
    }

    return 0;
}


void* sumar(void* id) {
    int k = 0;
    while(1) {
        pthread_mutex_lock(&mutex);
        printf("Thread with id %d is in the mutex zone and has increased the shared resource\n", *(int*) id);
        sleep(1);
        shared_resource += 100; 
        pthread_mutex_unlock(&mutex);
        for(int i = 0; i < 1000000; i++) {k = k+1; k = k - 1;}
    }
    return 0;
}

void* lectura(void* id) {
    int k = 0;
    while(1) {
        pthread_mutex_lock(&mutex);
        printf("Thread with id %d is in the mutex zone and has read the value %lu\n", *(int*) id, shared_resource);
        sleep(1);
        pthread_mutex_unlock(&mutex);
        for(int i = 0; i < 1000000; i++) {k = k+1; k = k - 1;}
    }
    return 0;
}