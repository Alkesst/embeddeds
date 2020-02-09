// execution taskset -c 0 
#include <pthread.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <errno.h>
// ----------------------------------------------------------------------------------------------------------------------------------------
// Configuraciones
// prioridades: deposito 1: 27
// deposito 2: 26
// monitor: 25
// main thread: 30

static inline long generate_random(long min, long max) {
    long r = min + random() % (max - min + 1);
    return r;
}

static pthread_attr_t generate_attr(int sched, int prio) {
    pthread_attr_t attr;
    // creamos el parametro con la prioridad
    struct sched_param param = { prio };
    pthread_attr_init(&attr);
    // PTHREAD_EXPLICIT_SCHED PTHread-Hebras inheritsched. Ponemos el scheduler a EXPLICIT;
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, sched);
    pthread_attr_setschedparam(&attr, &param);
    return attr;
}

static int change_priority(int sched, int prio) {
    struct sched_param param = {prio};
    return pthread_setschedparam(pthread_self(), sched, &param);
}

static int mutex_create(pthread_mutex_t* mutex) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    int r = pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return r;
}

// ----------------------------------------------------------------------------------------------------------------------------------------

// no hay timers -> no hay senyales por usar
typedef struct {
    pthread_t monitor, dep1, dep2;
    pthread_mutex_t mutex_dep1, mutex_dep2;
    int periodo_dep1, periodo_dep2;
    int temp_dep1, temp_dep2;
} Monitor;

#define MAX_TEMP 90
#define MIN_TEMP 10

void normalize_overflow(struct timespec* next) {
    if(next->tv_nsec >= 1000000000) {
        next->tv_nsec -= 1000000000;
        next->tv_sec++;
    }
}

void* dep1_task(void*);
void* dep2_task(void*);
void* monitor_task(void*);

int main() {
    mlockall(MCL_CURRENT | MCL_FUTURE);
    // prioridad del thread principal es 30
    change_priority(SCHED_FIFO, 30);
    srandom(time(NULL));
    Monitor monitor;
    mutex_create(&monitor.mutex_dep1);
    mutex_create(&monitor.mutex_dep2);
    monitor.periodo_dep1 = 1.5;
    monitor.periodo_dep2 = 2.5;

    pthread_attr_t t1, t2, t3;
    t1 = generate_attr(SCHED_FIFO, 25);
    t2 = generate_attr(SCHED_FIFO, 26);
    t3 = generate_attr(SCHED_FIFO, 27);

    pthread_create(&monitor.dep1, &t3, dep1_task, &monitor);
    pthread_create(&monitor.dep2, &t2, dep2_task, &monitor);
    pthread_create(&monitor.monitor, &t1, monitor_task, &monitor);
    
    pthread_attr_destroy(&t1);
    pthread_attr_destroy(&t2);
    pthread_attr_destroy(&t3);

    pthread_join(monitor.dep1, NULL);
    pthread_join(monitor.dep2, NULL);
    pthread_join(monitor.monitor, NULL);

    return 0;
}

void* dep1_task(void* arg)  {
    Monitor* monitor = (Monitor*) arg;
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    next.tv_sec += 1;
    next.tv_nsec += 500000000;
    normalize_overflow(&next);

    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);
        pthread_mutex_lock(&monitor->mutex_dep1);
        monitor->temp_dep1 = generate_random(MIN_TEMP, MAX_TEMP);
        if(monitor->temp_dep1 < MAX_TEMP * 0.8) {
            printf("[Dep1] - Temperatura optima en deposito 1\n");
        } else if (monitor->temp_dep1 >= MAX_TEMP * 0.8 && monitor->temp_dep1 < MAX_TEMP * 0.9) {
            printf("[Dep1] - Temperatura alta en deposito 1\n");
        } else if (monitor->temp_dep1 >= MAX_TEMP * 0.9) {
            printf("[Dep1] - Temperatura extrema en deposito 1\n");
        }
        pthread_mutex_unlock(&monitor->mutex_dep1);
        next.tv_sec += 1;
        next.tv_nsec += 500000000;
        normalize_overflow(&next);
    }
    
    return 0;
}

void* dep2_task(void* arg) {
    Monitor* monitor = (Monitor*) arg;
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    next.tv_sec += 2;
    next.tv_nsec += 500000000;
    normalize_overflow(&next);

    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);
        pthread_mutex_lock(&monitor->mutex_dep2);
        monitor->temp_dep2 = generate_random(MIN_TEMP, MAX_TEMP);
        if(monitor->temp_dep2 < MAX_TEMP * 0.8) {
            printf("[Dep2] - Temperatura optima en deposito 1\n");
        } else if (monitor->temp_dep2 >= MAX_TEMP * 0.8 && monitor->temp_dep2 < MAX_TEMP * 0.9) {
            printf("[Dep2] - Temperatura alta en deposito 1\n");
        } else if (monitor->temp_dep2 >= MAX_TEMP * 0.9) {
            printf("[Dep2] - Temperatura extrema en deposito 1\n");
        }
        pthread_mutex_unlock(&monitor->mutex_dep2);
        next.tv_sec += 2;
        next.tv_nsec += 500000000;
        normalize_overflow(&next);
    }
    
    return 0;
}

void* monitor_task(void* arg) {
    Monitor* monitor = (Monitor*) arg;
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    next.tv_sec += 4;

    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);
        pthread_mutex_lock(&monitor->mutex_dep1);
        long temp1 = monitor->temp_dep1;
        
        pthread_mutex_unlock(&monitor->mutex_dep1);
        long temp2 = monitor->temp_dep2;
        pthread_mutex_lock(&monitor->mutex_dep2);

        pthread_mutex_unlock(&monitor->mutex_dep2);
        printf("[MONITOR]: Temperatura del DEP1: %ld; Temperatura del DEP2: %ld;\n", temp1, temp2);

        next.tv_sec += 4;
    }

    return 0;
}