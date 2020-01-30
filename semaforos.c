#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#define THREADS 3

void* access_resource(void*);

sem_t sem;

int main() {
    pthread_t threads[THREADS];
    int idThreads[THREADS];
    int returnCode; 

    // init semaphore 1 and 1 because it can handle 1 petition and it's initializated in 1
    sem_init(&sem, 1, 1);
    for(long t = 0; t < THREADS; t++) {
        idThreads[t] = t;
        printf("Created thread with id %ld\n", t);
        returnCode = pthread_create(&threads[t], NULL, access_resource, &idThreads[t]);
        if(returnCode) {
            printf("ERROR pthread_create returned %d\n", returnCode);
            exit(-1);
        }
    }

    for(int t = 0; t < THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    return 0;
}

void* access_resource(void* id) {
    int i, k = 0, time;
    while(1) {
        printf("Thread %d Is waiting to enter into the mutex zone\n",(*(int*) id));
        sem_wait(&sem);
        time = (int) ((double) rand() / RAND_MAX * 2);
        printf("Thread %d has entered the mutex zone and it's going to sleep %d\n",(*(int*) id), time);
        sleep(time);
        printf("Thread %d has been awakened.\n",(*(int*) id));
        sem_post(&sem);
        printf("Thread %d id doing something outside the mutex zone\n", (*(int*) id));

        for(i = 0; i < 1000000; i++) {k = k+1; k = k - 1;}
    }
}