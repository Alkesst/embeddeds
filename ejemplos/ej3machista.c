#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#define THREADS 10

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t bath_is_full;
    int girls, bois;
} MutexStructure;


void going_to_bathroom(int, MutexStructure*);
void* thread_process(void*);
int condicion_bathroom(int, MutexStructure*);

int main() {
    MutexStructure* mutex = malloc(sizeof(MutexStructure));
    pthread_t threads[THREADS];
    int return_code;

    mutex->girls = 0;
    mutex->bois = 0;
    srand(time(NULL));   // Initialization, should only be called once.
    pthread_mutex_init(&mutex->mutex, NULL);
    pthread_cond_init(&mutex->bath_is_full, NULL);

    for(long t = 0; t < THREADS; t++) {
        return_code = pthread_create(&threads[t], NULL, thread_process, mutex);
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

void going_to_bathroom(int is_girl, MutexStructure* mutex) {
    int r = rand() % 500;
    if(is_girl) {
        printf("A girl wants to enter the bathroom.\n");
    } else {
        printf("A boi wants to enter the bathroom.\n");
    }
    pthread_mutex_lock(&mutex->mutex);
    while(condicion_bathroom(is_girl, mutex)) {
        printf("A %d is waiting to enter\n", is_girl);
        pthread_cond_wait(&mutex->bath_is_full, &mutex->mutex);
    }
    (is_girl)? mutex->girls++ : mutex->bois++;
    printf("Estoy meando bro xD junto a otras %d personas\n", (is_girl)? mutex->girls : mutex->bois);
    pthread_mutex_unlock(&mutex->mutex);
    usleep(r * 1000);
    pthread_mutex_lock(&mutex->mutex);
    printf("A %d is exiting\n", is_girl);
    (is_girl)? mutex->girls-- : mutex->bois--;
    pthread_cond_signal(&mutex->bath_is_full);
    pthread_mutex_unlock(&mutex->mutex);
    usleep(r * 1000);
} 

void* thread_process(void* mutex_void) {
    MutexStructure* mutex = (MutexStructure*) mutex_void;
    int t;
    while(1) {
        t = rand() % 2;
        going_to_bathroom(t, mutex);
    }
} 

int condicion_bathroom(int is_girl, MutexStructure* mutex) {
    if(is_girl) {
        return mutex->girls == 1 || mutex->bois > 0;
    } else {
        return mutex->bois == 5 || mutex->girls > 0;
    }
}