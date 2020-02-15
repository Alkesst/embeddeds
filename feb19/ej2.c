#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>
#include <errno.h>

static int create_thread(pthread_t* thread, void* (func)(), void* arg, int sched, int prio) {
    pthread_attr_t attr;
    struct sched_param param = { prio };
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, sched);
    pthread_attr_setschedparam(&attr, &param);
    int r = pthread_create(thread, &attr, func, arg);
    pthread_attr_destroy(&attr);
    return r;
}

static int chage_priority(int sched, int prio) {
    struct sched_param param = { prio };
    return pthread_setschedparam(pthread_self(), sched, &param);
}

static int create_mutex(pthread_mutex_t* mutex) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    int r = pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return r;
}

static inline long generate_random(pthread_mutex_t* mutex, long min, long max) {
    long r;
    pthread_mutex_lock(mutex);
    r = min + random() % (max - min + 1);
    pthread_mutex_unlock(mutex);
    return r;
}

#define MAX 90

// hab1: 28
// hab2: 27
// monitor: 26
// main :30

typedef struct {
    pthread_t thread;
    int periodo;
    long iluminacion;
    pthread_mutex_t mutex;
} Habitacion;

typedef struct {
    Habitacion hab1, hab2;
    pthread_t monitor;
    pthread_mutex_t random;
    int periodo;
} Monitor;

void* hab1(void*);
void* hab2(void*);
void* monitor(void*);


int main() {
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srandom(time(NULL));
    chage_priority(SCHED_FIFO, 30);
    Monitor mon;
    mon.hab1.periodo = 2;
    mon.hab2.periodo = 3;
    mon.periodo = 4;
    create_mutex(&mon.random);
    create_mutex(&mon.hab1.mutex);
    create_mutex(&mon.hab2.mutex);

    create_thread(&mon.hab1.thread, hab1, &mon, SCHED_FIFO, 28);
    create_thread(&mon.hab2.thread, hab2, &mon, SCHED_FIFO, 27);
    create_thread(&mon.monitor, monitor, &mon, SCHED_FIFO, 26);

    pthread_join(mon.monitor, NULL);
    pthread_join(mon.hab1.thread, NULL);
    pthread_join(mon.hab2.thread, NULL);

    pthread_mutex_destroy(&mon.hab1.mutex);
    pthread_mutex_destroy(&mon.hab2.mutex);
    pthread_mutex_destroy(&mon.random);

    return 0;
}



void* hab1(void* arg) {
    Monitor* mon = (Monitor*) arg;
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    tspec.tv_sec += mon->hab1.periodo;
    while(1) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, NULL) == EINTR);
        pthread_mutex_lock(&mon->hab1.mutex);
        mon->hab1.iluminacion = generate_random(&mon->random, 50, MAX);
        if(mon->hab1.iluminacion < MAX * 0.8) {
            printf("[Hab1]: Activacion de la luz artificial\n");
        } else if (mon->hab1.iluminacion >= MAX * 0.8 && mon->hab1.iluminacion <= MAX*0.9) {
            printf("[Hab1]: Luz Optima\n");    
        } else if (mon->hab1.iluminacion > MAX * 0.9) {
            printf("[Hab1]: Luz Extrema. Activacion de cortinas\n");    
        }
        pthread_mutex_unlock(&mon->hab1.mutex);
        tspec.tv_sec += mon->hab1.periodo;
    }
}

void* hab2(void* arg) {
    Monitor* mon = (Monitor*) arg;
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    tspec.tv_sec += mon->hab2.periodo;
    while(1) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, NULL) == EINTR);
        pthread_mutex_lock(&mon->hab2.mutex);
        mon->hab2.iluminacion = generate_random(&mon->random, 50, MAX);
        if(mon->hab2.iluminacion < MAX * 0.8) {
            printf("[Hab2]: Activacion de la luz artificial\n");
        } else if (mon->hab2.iluminacion >= MAX * 0.8 && mon->hab2.iluminacion <= MAX*0.9) {
            printf("[Hab2]: Luz Optima\n");    
        } else if (mon->hab2.iluminacion > MAX * 0.9) {
            printf("[Hab2]: Luz Extrema. Activacion de cortinas\n");    
        }
        pthread_mutex_unlock(&mon->hab2.mutex);
        tspec.tv_sec += mon->hab2.periodo;
    }
}

void* monitor(void* arg) {
    Monitor* mon = (Monitor*) arg;
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    tspec.tv_sec += mon->periodo;
    long hab1, hab2;
    while(1) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, NULL) == EINTR);
        pthread_mutex_lock(&mon->hab1.mutex);
        hab1 = mon->hab1.iluminacion;
        pthread_mutex_unlock(&mon->hab1.mutex);

        pthread_mutex_lock(&mon->hab2.mutex);
        hab2 = mon->hab2.iluminacion;
        pthread_mutex_unlock(&mon->hab2.mutex);

        printf("[Monitor]: Iluminacion Habitacion 1: %ld, Iluminacion Habitacion 2: %ld\n", hab1, hab2);

        tspec.tv_sec += mon->periodo;
    }
}