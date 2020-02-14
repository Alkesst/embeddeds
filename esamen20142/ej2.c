#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sched.h>
#include <time.h>
#include <errno.h>

// configuration:
static int create_thread(pthread_t* thread, void* (func)(), void* args, int sched, int prio) {
    pthread_attr_t attr;
    struct sched_param param = { prio };
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, sched);
    pthread_attr_setschedparam(&attr, &param);
    int r = pthread_create(thread, &attr, func, args);
    pthread_attr_destroy(&attr);
    return r;
}

static int change_priority(int sched, int prio) {
    struct sched_param param = { prio };
    return pthread_setschedparam(pthread_self(), sched, &param);
}

static inline long generate_random(pthread_mutex_t* mutex, long min, long max) {
    long r;
    pthread_mutex_lock(mutex);
    r = min + random() % (max - min + 1);
    pthread_mutex_unlock(mutex);
    return r;
}

static int create_mutex(pthread_mutex_t* mutex) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    int r = pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return r;
}

void normalize_overflow(struct timespec* next) {
    if(next->tv_nsec >= 1000000000) {
        next->tv_nsec -= 1000000000;
        next->tv_sec++;
    }
}

#define MAX_TEMP 90

typedef struct {
    pthread_t thread;
    int periodo_sec, periodo_nsec;
    pthread_mutex_t mutex;
    long temp;
} Deposito;

typedef struct {
    Deposito dep1, dep2;
    pthread_t monitor;
    int periodo;
    pthread_mutex_t random;
} Control;

void* control_dep1(void*);
void* control_dep2(void*);
void* monitor_task(void*);

int main() {
    mlockall(MCL_CURRENT | MCL_FUTURE);
    Control ctrl;
    create_mutex(&ctrl.random);    
    create_mutex(&ctrl.dep1.mutex);    
    create_mutex(&ctrl.dep2.mutex);
    change_priority(SCHED_FIFO, 35);
    ctrl.dep1.periodo_sec = 1;
    ctrl.dep1.periodo_nsec = 500000000;

    ctrl.dep2.periodo_sec = 2;
    ctrl.dep2.periodo_nsec = 500000000;

    ctrl.periodo = 4;
    create_thread(&ctrl.dep1.thread, control_dep1, &ctrl, SCHED_FIFO, 28);
    create_thread(&ctrl.dep2.thread, control_dep2, &ctrl, SCHED_FIFO, 27);
    create_thread(&ctrl.monitor, monitor_task, &ctrl, SCHED_FIFO, 26);

    pthread_join(ctrl.dep1.thread, NULL);
    pthread_join(ctrl.dep2.thread, NULL);
    pthread_join(ctrl.monitor, NULL);

    pthread_mutex_destroy(&ctrl.dep1.mutex);
    pthread_mutex_destroy(&ctrl.dep2.mutex);
    pthread_mutex_destroy(&ctrl.random);
    
    return 0;
}



void* control_dep1(void* arg) {
    Control* ctrl = (Control*) arg;
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    tspec.tv_sec += ctrl->dep1.periodo_sec;
    tspec.tv_nsec += ctrl->dep1.periodo_nsec;
    normalize_overflow(&tspec);
    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, NULL) == EINTR);
        pthread_mutex_lock(&ctrl->dep1.mutex);
        ctrl->dep1.temp  = generate_random(&ctrl->random, 35, MAX_TEMP);
        pthread_mutex_unlock(&ctrl->dep1.mutex);
        if(ctrl->dep1.temp  < MAX_TEMP * 0.80) {
            printf("[Dep1]: Temperatura optima\n");
        } else if(ctrl->dep1.temp  >= MAX_TEMP * 0.80 && ctrl->dep1.temp  < MAX_TEMP * 0.90) {
            printf("[Dep1]: Temperatura alta\n");
        } else if(ctrl->dep1.temp  >= MAX_TEMP * 0.90) {
            printf("[Dep1]: Well fuck\n");
        }
        tspec.tv_sec += ctrl->dep1.periodo_sec;
        tspec.tv_nsec += ctrl->dep1.periodo_nsec;
        normalize_overflow(&tspec);
    }
}

void* control_dep2(void* arg) {
    Control* ctrl = (Control*) arg;
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    tspec.tv_sec += ctrl->dep2.periodo_sec;
    tspec.tv_nsec += ctrl->dep2.periodo_nsec;
    normalize_overflow(&tspec);
    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, NULL) == EINTR);
        
        pthread_mutex_lock(&ctrl->dep2.mutex);
        ctrl->dep2.temp  = generate_random(&ctrl->random, 35, MAX_TEMP);
        pthread_mutex_unlock(&ctrl->dep2.mutex);
        if(ctrl->dep2.temp  < MAX_TEMP * 0.80) {
            printf("[Dep2]: Temperatura optima\n");
        } else if(ctrl->dep2.temp  >= MAX_TEMP * 0.80 && ctrl->dep2.temp  < MAX_TEMP * 0.90) {
            printf("[Dep2]: Temperatura alta\n");
        } else if(ctrl->dep2.temp  >= MAX_TEMP * 0.90) {
            printf("[Dep2]: Well fuck\n");
        }
        tspec.tv_sec += ctrl->dep2.periodo_sec;
        tspec.tv_nsec += ctrl->dep2.periodo_nsec;
        normalize_overflow(&tspec);
    }
}

void* monitor_task(void* arg) {
    Control* ctrl = (Control*) arg;
    struct timespec tspec;
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    tspec.tv_sec += ctrl->periodo;
    long dep1t, dep2t;
    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, NULL) == EINTR);
        pthread_mutex_lock(&ctrl->dep1.mutex);
        dep1t = ctrl->dep1.temp;
        pthread_mutex_unlock(&ctrl->dep1.mutex);

        pthread_mutex_lock(&ctrl->dep1.mutex);
        pthread_mutex_unlock(&ctrl->dep1.mutex);
        dep2t = ctrl->dep2.temp;
        printf("[Monitor]: Temperatura Dep1: %ld, Temperatura Dep2: %ld\n", dep1t, dep2t);
        tspec.tv_sec += ctrl->periodo;
    }
}