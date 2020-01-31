#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#define BUFFER_SIZE 10

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t buffer_not_filled;
    pthread_cond_t buffer_not_empty;
    int count, first, last;
    int buffer[BUFFER_SIZE];
} buffer;

// It's something like that I guess
// https://github.com/jesuspa98/PSC_Exercises/blob/ff681c40704a861b8bcf022d9b7f55c81165dcb7/Alec/Concurrencia/Concurrencia2018/src/miExamen/semaforos/Bar.java#L19
int meter(int item, buffer *buffer) {
    int k = 0;
    printf("Producer is going to enter mutex\n");
    pthread_mutex_lock(&buffer->mutex);
    while(buffer->count == BUFFER_SIZE) {
        printf("producer waiting...\n");
        pthread_cond_wait(&buffer->buffer_not_filled, &buffer->mutex);
    }
    printf("Producer has created the value %d\n", item);
    buffer->buffer[buffer->last] = item;
    buffer->last = (buffer->last + 1) % BUFFER_SIZE;
    buffer->count++;
    pthread_cond_signal(&buffer->buffer_not_empty);
    pthread_mutex_unlock(&buffer->mutex);
    for(int i = 0; i < 1000000; i++) {k = k+1; k = k - 1;}
    return 0;
}

int sacar(buffer* buffer) {
    int k = 0;
    printf("Consumer is going to enter mutex\n");
    pthread_mutex_lock(&buffer->mutex);
    while(buffer->count == 0) {
        printf("consumer waiting...\n");
        pthread_cond_wait(&buffer->buffer_not_empty, &buffer->mutex);
    }
    printf("Consumer has consumed the value %d\n", buffer->buffer[buffer->first]);
    buffer->first = (buffer->first + 1) % BUFFER_SIZE;
    buffer->count--;
    pthread_cond_signal(&buffer->buffer_not_filled);
    pthread_mutex_unlock(&buffer->mutex);
    for(int i = 0; i < 1000000; i++) {k = k+1; k = k - 1;}
    return 0;
}

void* producer_routine(void* buffer_void) {
    
    buffer* b = (buffer*) buffer_void;
    srand(time(NULL));   // Initialization, should only be called once.
    int r; 
    while(1) {
        r = rand() % 101;
        meter(r, b);
    }
    return 0;
}


void* consumer_routine(void* buffer_void) {
    // since buffer it's a shared resource initialized in the main
    // we don't need the "value" stored in that memory address, we need
    // the pointer to the object.
    // when we do *(buffer*) we're making a copy of that memory, 
    // with (buffer*) we're just casting the void* pointer to a buffer* pointer;
    buffer* b = (buffer*) buffer_void;
    while(1) {
        // we don't need & since b is already a pointer. More exactly it's the memory address of the buffer
        // we created in the main process.
        sacar(b);
    }
    return 0;
}

void buffer_init(buffer* buffer) {
    buffer->count = 0;
    buffer->first = 0;
    buffer->last = 0;
    // we can do it using semaphores or using pthread conditions
    pthread_cond_init(&buffer->buffer_not_filled, NULL);
    pthread_cond_init(&buffer->buffer_not_empty, NULL);
    pthread_mutex_init(&buffer->mutex, NULL);
}

int main() {
    pthread_t productor, consumidor;
    buffer b;
    buffer_init(&b);
    pthread_create(&productor, NULL, producer_routine, &b);
    pthread_create(&consumidor, NULL, consumer_routine, &b);
    pthread_join(productor, NULL);
    pthread_join(consumidor, NULL);
    return 0;
}