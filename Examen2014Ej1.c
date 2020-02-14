// Include
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/types.h>

// Define
#define TORNO_ENTRADA SIGRTMIN
#define TORNO_SALIDA SIGRTMIN + 1
#define TIMER_TORNO_1 SIGRTMIN + 2
#define TIMER_TORNO_2 SIGRTMIN + 3

// Structs
typedef struct {
    pthread_t thread;
    int periodo;
} Torno;

typedef struct {
    pthread_t monitor;
    Torno entrada, salida;
    int personas_dentro;
    pthread_mutex_t random_mutex;
} Ej1;

// Configuracion
static int create_timer(timer_t* timer, int signo) {
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = signo;
    sigev.sigev_value.sival_int = 0;
    return timer_create(CLOCK_MONOTONIC, &sigev, timer);
}

static int activate_timer(timer_t* timer, time_t sec, long nsec) {
    struct itimerspec it;
    it.it_interval.tv_sec = sec;
    it.it_interval.tv_sec = nsec;
    it.it_value.tv_sec = 0;
    it.it_value.tv_nsec = 1;
    return timer_settime(timer, 0, &it, NULL);
}

static int create_thread(pthread_t* thread, void* (func)(), void* args, int sched, int prio) {
    pthread_attr_t attr;
    struct sched_param param = {prio};
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, sched);
    pthread_attr_setschedparam(&attr, &param);
    int r = pthread_create(thread, &attr, func, args);
    pthread_attr_destroy(&attr);
    return r;
}

static int change_priority(int sched, int prio) {
    struct sched_param param = {prio};
    return pthread_setschedparam(pthread_self(), sched, &param);
}

static inline int generate_random(pthread_mutex_t* mutex, long min, long max) {
    long res;
    pthread_mutex_lock(mutex);
    res = min + random() % (max - min + 1);
    pthread_mutex_unlock(mutex);
    return res;
}

sigset_t sigset;

void* torno_entrada(void*);
void* torno_salida(void*);
void* monitor_task(void*);

int main() {
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srandom(time(NULL));
    sigemptyset(&sigset);
    sigaddset(&sigset, TORNO_ENTRADA);
    sigaddset(&sigset, TORNO_SALIDA);
    sigaddset(&sigset, TIMER_TORNO_1);
    sigaddset(&sigset, TIMER_TORNO_2);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    Ej1 stru;
    stru.entrada.periodo = 1;
    stru.salida.periodo = 2;
    stru.personas_dentro = 0;
    pthread_mutex_init(&stru.random_mutex, NULL);
    
    change_priority(SCHED_FIFO, 30);
    create_thread(&stru.entrada.thread, torno_entrada, &stru, SCHED_FIFO, 26);
    create_thread(&stru.salida.thread, torno_salida, &stru, SCHED_FIFO, 26);
    create_thread(&stru.monitor, monitor_task, &stru, SCHED_FIFO, 26);

    pthread_join(stru.monitor, NULL);
    pthread_join(stru.entrada.thread, NULL);
    pthread_join(stru.salida.thread, NULL);

    pthread_mutex_destroy(&stru.random_mutex);
    
    return 0;
}

void* torno_entrada(void* args) {
    Ej1* stru = (Ej1*) args;
    timer_t timer;
    sigset_t sigset_ti;
    sigemptyset(&sigset_ti);
    sigaddset(&sigset_ti, TIMER_TORNO_1);

    create_timer(&timer, TIMER_TORNO_1);
    activate_timer(&timer, stru->entrada.periodo, 0);

    while(1) {
        int info;
        sigwait(&sigset_ti, &info);
        if (info == TIMER_TORNO_1) {
            long r = generate_random(&stru->random_mutex, 0, 100);
            if (r < 60) {
                kill(getpid(), TORNO_ENTRADA);
            }
        }
    }
    timer_delete(&timer);
    return 0;
}

void* torno_salida(void* args) {
    Ej1* stru = (Ej1*) args;
    timer_t timer;
    sigset_t sigset_ti;
    sigemptyset(&sigset_ti);
    sigaddset(&sigset_ti, TIMER_TORNO_2);

    create_timer(&timer, TIMER_TORNO_2);
    activate_timer(&timer, stru->salida.periodo, 0);

    while(1) {
        int info;
        sigwait(&sigset_ti, &info);
        if (info == TIMER_TORNO_2) {
            long r = generate_random(&stru->random_mutex, 0, 100);
            if (r < 40) {
                kill(getpid(), TORNO_SALIDA);
            }
        }
    }
    timer_delete(&timer);
    return 0;
}

void* monitor_task(void* args) {
    Ej1* stru = (Ej1*) args;
    sigset_t sigset_ti;
    sigemptyset(&sigset_ti);
    sigaddset(&sigset_ti, TORNO_SALIDA);
    sigaddset(&sigset_ti, TORNO_ENTRADA);

    while(1) {
        int info;
        sigwait(&sigset_ti, &info);
        if(info == TORNO_ENTRADA) {
            stru->personas_dentro++;
            printf("Alguien ha entrado. Quedan %d\n", stru->personas_dentro);
        } else if (info == TORNO_SALIDA) {
            stru->personas_dentro--;
            printf("Alguien ha salido. Quedan %d\n", stru->personas_dentro);
        }
    }
    return 0;
}