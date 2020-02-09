#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

// CONFIGS

static int create_thread(pthread_t* thread, void* (func)(), int sched, int prio) {
    pthread_attr_t attr;
    struct sched_param param = {prio};
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, sched);
    pthread_attr_setschedparam(&attr, &param);
    int r = pthread_create(thread, &attr, func, NULL);
    pthread_attr_destroy(&attr);
    return r;
}

static int change_priority(int sched, int prio) {
    struct sched_param param = {prio};
    return pthread_setschedparam(pthread_self(), sched, &param);
}

// definitions

#define MAX 40

pthread_mutex_t mutex;
int shared_resource;

void* add_task();
void* sub_task();

// we're not using 

int main() {
    pthread_t a, b;
    create_thread(&a, add_task, SCHED_RR, 26);
    create_thread(&b, sub_task, SCHED_RR, 27);
    // highest priority to the main thread
    change_priority(SCHED_RR, 30);

    pthread_join(a, NULL);
    pthread_join(b, NULL);

    return 0;
}

void* add_task() {
    int i = 0;
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    next.tv_sec += 1;
    while(i < MAX) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);
        pthread_mutex_lock(&mutex);
        shared_resource++;
        printf("[ADD TASK]: %d. Iteration number: %d\n", shared_resource, i);
        pthread_mutex_unlock(&mutex);
        next.tv_sec += 1;
        i++;
    }
    return 0;
}

void* sub_task() {
    int i = 0;
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    next.tv_sec += 1;
    while(i < MAX) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);
        pthread_mutex_lock(&mutex);
        shared_resource--;
        printf("[SUB TASK]: %d. Iteration number: %d\n", shared_resource, i);
        pthread_mutex_unlock(&mutex);
        next.tv_sec += 1;
        i++;
    }
    return 0;
}