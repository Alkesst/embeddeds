#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

static int create_timer(timer_t* timer, int signo) {
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = signo;
    sigev.sigev_value.sival_int = 0;
    return timer_create(CLOCK_MONOTONIC, &sigev, timer);
}

static int activate_timer(timer_t timer, time_t secs, long nsec) {
    struct itimerspec itspec;
    itspec.it_interval.tv_sec = secs;
    itspec.it_interval.tv_nsec = nsec;
    itspec.it_value.tv_sec = 0;
    itspec.it_value.tv_nsec = 1;
    return timer_settime(timer, 0, &itspec, NULL);
}

static inline long generate_random(pthread_mutex_t* mutex) {
    long r;
    pthread_mutex_lock(mutex);
    r = random() % 2;
    pthread_mutex_unlock(mutex);
    return r;
}


#define ENTRADA_COCHE SIGRTMIN
#define ENTRADA_FURGO SIGRTMIN + 1
#define SALIDA_COCHE SIGRTMIN + 2
#define SALIDA_FURGO SIGRTMIN + 3
#define TIMER_COCHE SIGRTMIN + 4
#define TIMER_FURGO SIGRTMIN + 5
#define TIMER_SALIDA SIGRTMIN + 6

typedef struct {
    pthread_t thread;
    int periodo;
} Vehiculo;

typedef struct {
    Vehiculo furgo, coche;
    pthread_t entrada;
    pthread_t salida;
    int periodo;
    pthread_mutex_t random, mutex;
    int coches, furgos;
} Control;

sigset_t sigset;
void* coche_task(void*);
void* furgo_task(void*);
void* entrada_task(void*);
void* salida_task(void*);

int main() {
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srandom(time(NULL));
    sigemptyset(&sigset);
    sigaddset(&sigset, TIMER_SALIDA);
    sigaddset(&sigset, SALIDA_COCHE);
    sigaddset(&sigset, SALIDA_FURGO);
    sigaddset(&sigset, ENTRADA_COCHE);
    sigaddset(&sigset, ENTRADA_FURGO);
    sigaddset(&sigset, TIMER_FURGO);
    sigaddset(&sigset, TIMER_COCHE);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    Control c;
    c.coches = 0;
    c.furgos = 0;
    c.coche.periodo = 2;
    c.furgo.periodo = 3;
    c.periodo = 2;
    pthread_mutex_init(&c.random, NULL);
    pthread_mutex_init(&c.mutex, NULL);
    
    pthread_create(&c.coche.thread, NULL, coche_task, &c);
    pthread_create(&c.furgo.thread, NULL, furgo_task, &c);
    pthread_create(&c.entrada, NULL, entrada_task, &c);
    pthread_create(&c.salida, NULL, salida_task, &c);

    pthread_join(c.coche.thread, NULL);
    pthread_join(c.furgo.thread, NULL);
    pthread_join(c.entrada, NULL);
    pthread_join(c.salida, NULL);
    
    
    pthread_mutex_destroy(&c.random);
    pthread_mutex_destroy(&c.mutex);
    return 0;
}



void* coche_task(void* arg) {
    Control* c = (Control*) arg;
    timer_t timer;
    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, TIMER_COCHE);
    create_timer(&timer, TIMER_COCHE);
    activate_timer(timer, c->coche.periodo, 0);
    while(true) {
        int info;
        sigwait(&thread_set, &info);
        if(info == TIMER_COCHE) {
            kill(getpid(), ENTRADA_COCHE);
        }
    }
    timer_delete(timer);
}

void* furgo_task(void* arg) {
    Control* c = (Control*) arg;
    timer_t timer;
    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, TIMER_FURGO);
    create_timer(&timer, TIMER_FURGO);
    activate_timer(timer, c->furgo.periodo, 0);
    while(true) {
        int info;
        sigwait(&thread_set, &info);
        if(info == TIMER_FURGO) {
            kill(getpid(), ENTRADA_FURGO);
        }
    }
    timer_delete(timer);
}

void* entrada_task(void* arg) {
    Control* c = (Control*) arg;
    timer_t timer;
    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, TIMER_SALIDA);
    sigaddset(&thread_set, ENTRADA_COCHE);
    sigaddset(&thread_set, ENTRADA_FURGO);
    create_timer(&timer, TIMER_SALIDA);
    activate_timer(timer, c->furgo.periodo, 0);
    while(true) {
        int info;
        sigwait(&thread_set, &info);
        if(info == TIMER_SALIDA) {
            long r = generate_random(&c->random);
            (r) ? kill(getpid(), SALIDA_COCHE): kill(getpid(), SALIDA_FURGO);
        } else if(info == ENTRADA_COCHE) {
            pthread_mutex_lock(&c->mutex);
            c->coches++;
            printf("Ha entrado un coche hay %d coches\n", c->coches);
            pthread_mutex_unlock(&c->mutex);
        } else if(info == ENTRADA_FURGO) {
            pthread_mutex_lock(&c->mutex);
            c->furgos++;
            printf("Ha entrado una furgo hay %d furgos\n", c->furgos);
            pthread_mutex_unlock(&c->mutex);
        }
    }

    timer_delete(timer);
}

void* salida_task(void* arg) {
    Control* c = (Control*) arg;
    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, SALIDA_COCHE);
    sigaddset(&thread_set, SALIDA_FURGO);
    while(true) {
        int info;
        sigwait(&thread_set, &info);
        pthread_mutex_lock(&c->mutex);
        if(info == SALIDA_COCHE && c->coches > 0) {
            c->coches--;
            printf("Ha salido un coche hay %d coches\n", c->coches);
        } else if (info == SALIDA_FURGO && c->furgos > 0) {
            c->furgos--;
            printf("Ha salido una furgo hay %d furgos\n", c->furgos);
        }
        pthread_mutex_unlock(&c->mutex);
    }
}