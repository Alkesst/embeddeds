#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
// imanu;

// EINTR
// configs:

/*static int create_thread(pthread_t* thread, void* (func) (), void* arg, int sched, int prio) {
    pthread_attr_t attr;
    struct sched_param param = { prio };
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, sched);
    pthread_attr_setschedparam(&attr, &param);
    int r = pthread_create(thread, &attr, func, arg);
    pthread_attr_destroy(&attr);
    return r;
}*/

static int create_timer(timer_t* timer, int signo) {
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = signo;
    sigev.sigev_value.sival_int = 0;
    return timer_create(CLOCK_MONOTONIC, &sigev, timer);
}

static int activate_timer(timer_t timer, time_t sec, long nsec) {
    struct itimerspec itspec;
    itspec.it_interval.tv_sec = sec;
    itspec.it_interval.tv_nsec = nsec;
    itspec.it_value.tv_sec = 0;
    itspec.it_value.tv_nsec = 1;
    return timer_settime(timer, 0, &itspec, NULL);
}

static inline long generate_random(pthread_mutex_t* mutex, long min, long max) {
    long r;
    pthread_mutex_lock(mutex);
    r = min + random() % (max - min + 1);
    pthread_mutex_unlock(mutex);
    return r;
}

#define ENTRADA SIGRTMIN 
#define SALIDA SIGRTMIN + 1
#define TIMER_VENTRADA SIGRTMIN + 2
#define TIMER_VSALIDA SIGRTMIN + 3

typedef struct {
    pthread_t thread;
    int periodo;
} Valvula;

typedef struct {
    int litros;
    Valvula entrada, salida;
    pthread_t control;
    pthread_mutex_t random;
} Control;

void* valvula_entrada(void*);
void* valvula_salida(void*);
void* controller_task(void*);

sigset_t sigset;

int main() {
    mlockall(MCL_FUTURE | MCL_CURRENT);
    srandom(time(NULL));
    Control ctrl;
    ctrl.entrada.periodo = 1;
    ctrl.salida.periodo = 2;
    pthread_mutex_init(&ctrl.random, NULL);

    sigemptyset(&sigset);
    sigaddset(&sigset, TIMER_VENTRADA);
    sigaddset(&sigset, TIMER_VSALIDA);
    sigaddset(&sigset, ENTRADA);
    sigaddset(&sigset, SALIDA);

    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    
    //create_thread(&ctrl.entrada.thread, valvula_entrada, &ctrl, SCHED_FIFO, 26);
    //create_thread(&ctrl.salida.thread, valvula_entrada, &ctrl, SCHED_FIFO, 26);
    //create_thread(&ctrl.control, controller_task, &ctrl, SCHED_FIFO, 26);
    pthread_create(&ctrl.entrada.thread, NULL, valvula_entrada, &ctrl);
    pthread_create(&ctrl.salida.thread, NULL, valvula_salida, &ctrl);
    pthread_create(&ctrl.control, NULL, controller_task, &ctrl);

    pthread_join(ctrl.salida.thread, NULL);
    pthread_join(ctrl.entrada.thread, NULL);
    pthread_join(ctrl.control, NULL);
    
    pthread_mutex_destroy(&ctrl.random);

    return 0;
}



void* valvula_entrada(void* arg) {
    Control* ctrl = (Control*) arg;
    timer_t timer;
    create_timer(&timer, TIMER_VENTRADA);
    activate_timer(timer, ctrl->entrada.periodo, 0);
    sigset_t sig_thread;
    sigemptyset(&sig_thread);
    sigaddset(&sig_thread, TIMER_VENTRADA);
    while(1) {
        int info;
        sigwait(&sig_thread, &info);
        if(info == TIMER_VENTRADA) {
            long r = generate_random(&ctrl->random, 0, 100);
            if(r <= 60) {
                kill(getpid(), ENTRADA);
                printf("[ValvE]: Se abre\n");
            }
        }
    }
    timer_delete(timer);
}

void* valvula_salida(void* arg) {
    Control* ctrl = (Control*) arg;
    timer_t timer;
    create_timer(&timer, TIMER_VSALIDA);
    activate_timer(timer, ctrl->salida.periodo, 0);
    sigset_t sig_thread;
    sigemptyset(&sig_thread);
    sigaddset(&sig_thread, TIMER_VSALIDA);
    while(1) {
        int info;
        sigwait(&sig_thread, &info);
        if(info == TIMER_VSALIDA) {
            long r = generate_random(&ctrl->random, 0, 100);
            if(r <= 40) {
                kill(getpid(), SALIDA);
                printf("[ValvS]: Se abre\n");
            }
        }
    }
    timer_delete(timer);
}

void* controller_task(void* arg) {
    Control* ctrl = (Control*) arg;
    sigset_t sig_thread;
    sigemptyset(&sig_thread);
    sigaddset(&sig_thread, ENTRADA);
    sigaddset(&sig_thread, SALIDA);
    while(1) {
        int info;
        sigwait(&sig_thread, &info);
        if(info == ENTRADA) {
            ctrl->litros++;
            printf("[Controlador]: Ha entrado 1 litro de agua. Hay %d litros\n", ctrl->litros);
        } else if(info == SALIDA && ctrl->litros > 0) {
            ctrl->litros--;
            printf("[Controlador]: Ha salido 1 litro de agua. Hay %d litros\n", ctrl->litros);
        }
    }
}