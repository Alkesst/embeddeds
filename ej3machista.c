#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#define THREADS 10

pthread_mutex_t mutex;
sem_t semaphore_bois;
sem_t semaphore_girls;

typedef struct {
    sem_t semaphore;
    int id;
    int girls;
    int bois;
} ThreadParameters;

void* want_enter_boi(void*);
void* want_enter_girls(void*);

int main() {
    pthread_t thread_bois[THREADS];
    pthread_t thread_girls[THREADS];
    ThreadParameters parameters[THREADS*2];
    int return_code;
    int bois_index, girls_index; 

    pthread_mutex_init(&mutex, NULL);
    sem_init(&semaphore_girls, 1, 1);
    sem_init(&semaphore_bois, 5, 5);

    for(long t = 0; t < THREADS; t++) {
        bois_index = 2 * t;
        girls_index = 2* t + 1;
        parameters[bois_index].id = bois_index;
        parameters[girls_index].id = girls_index;
        parameters[bois_index].semaphore = semaphore_bois;
        parameters[girls_index].semaphore = semaphore_girls;
        return_code = pthread_create(&thread_bois[t], NULL, want_enter_boi, &parameters[bois_index]);
        if(return_code) {
            printf("WELL, something went wrong creating the thread %d return code has been returned.", return_code);
            exit(-1);
        }
        return_code = pthread_create(&thread_girls[t], NULL, want_enter_girls, &parameters[girls_index]);
        if(return_code) {
            printf("WELL, something went wrong creating the thread %d return code has been returned.", return_code);
            exit(-1);
        }
    }

    for(int t = 0; t < THREADS; t++) {
        pthread_join(thread_bois[t], NULL);
        pthread_join(thread_girls[t], NULL);
    }

    return 0;
}

void* want_enter_boi(void* params_void) {
    ThreadParameters params = *(ThreadParameters*) params_void;
    int k = 0;
    while(1) {
        pthread_mutex_lock(&mutex);
        printf("The boi %d has entered the bathroom", params.id);
        sleep(1);
        pthread_mutex_unlock(&mutex);
        for(int i = 0; i < 1000000; i++) {k = k+1; k = k - 1;}
    }
}
