#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>

static int create_thread(pthread_t* thread, void* (func)(), void* argg, int sched, int prio) {
    struct sched_param param = { prio };
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, sched);
    pthread_attr_setschedparam(&attr, &param);
    int r = pthread_create(thread, &attr, func, argg);
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

#define MAX 90

typedef struct { 
    pthread_t thread;
    int periodo;
    int valor;
    pthread_mutex_t mutex;
} Sensor;

typedef struct {
    pthread_t control;
    pthread_mutex_t random;
    Sensor presencia, vibracion;
    int periodo;
} Control;

void* presencia_task(void*);
void* vibracion_task(void*);
void* control_task(void*);

int main() {
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srand(time(NULL));
    Control c;
    create_mutex(&c.presencia.mutex);
    create_mutex(&c.vibracion.mutex);
    pthread_mutex_init(&c.random, NULL);
    change_priority(SCHED_FIFO, 30);
    c.periodo = 4;
    c.presencia.periodo = 2;
    c.vibracion.periodo = 3;

    create_thread(&c.control, control_task, &c, SCHED_FIFO, 25);
    create_thread(&c.presencia.thread, presencia_task, &c, SCHED_FIFO, 29);
    create_thread(&c.vibracion.thread, vibracion_task, &c, SCHED_FIFO, 27);

    pthread_join(c.control, NULL);
    pthread_join(c.presencia.thread, NULL);
    pthread_join(c.vibracion.thread, NULL);

    pthread_mutex_destroy(&c.presencia.mutex);
    pthread_mutex_destroy(&c.vibracion.mutex);
    pthread_mutex_destroy(&c.random);

    return 0;
}


void* presencia_task(void* argg) {
    Control* c = (Control*) argg;
    struct timespec ispec; 
    clock_gettime(CLOCK_MONOTONIC, &ispec);
    ispec.tv_sec += c->presencia.periodo;
    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->presencia.mutex);
        c->presencia.valor = generate_random(&c->random, 0, 1);
        if(c->presencia.valor) {
            printf("[Sensor Presencia]: Posible presencia\n");
        }   
        pthread_mutex_unlock(&c->presencia.mutex);
        ispec.tv_sec += c->presencia.periodo;
    }
}

void* vibracion_task(void* argg) {
    Control* c = (Control*) argg;
    struct timespec ispec; 
    clock_gettime(CLOCK_MONOTONIC, &ispec);
    ispec.tv_sec += c->vibracion.periodo;
    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->vibracion.mutex);
        c->vibracion.valor = generate_random(&c->random, 0, MAX * 2);
        if(c->vibracion.valor > MAX) {
            printf("[Sensor Presencia]: Posible entrada por ventana\n");
        }   
        pthread_mutex_unlock(&c->vibracion.mutex);
        ispec.tv_sec += c->vibracion.periodo;
    }
}

void* control_task(void* argg) {
    Control* c = (Control*) argg;
    struct timespec ispec; 
    clock_gettime(CLOCK_MONOTONIC, &ispec);
    ispec.tv_sec += c->periodo;
    long vib, pres;
    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->vibracion.mutex);
        vib = c->vibracion.valor;
        pthread_mutex_unlock(&c->vibracion.mutex);
        pthread_mutex_lock(&c->presencia.mutex);
        pres = c->presencia.valor;
        pthread_mutex_unlock(&c->presencia.mutex);

        printf("[Control]: Nivel de Presencia: %ld; Nivel de vibracion: %ld\n", pres, vib);
        ispec.tv_sec += c->periodo;
    }
}