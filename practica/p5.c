#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>


// configuration 
static int create_thread(pthread_t* thread, void* (func)(), void* arg, int sched, int prio) {
    pthread_attr_t attr;
    struct sched_param param = { prio };
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, sched);
    pthread_attr_setschedparam(&attr, &param);
    int r = pthread_creat(thread, &attr, func, arg);
    pthread_attr_destroy(&attr);
    return r;
}

static int change_priority(int sched, int prio) {
    struct sched_param param = { prio };
    return pthread_setschedparam(pthread_self(), sched, &param);
}

static int create_timer(timer_t* timer, int signo) {
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = signo;
    sigev.sigev_value.sigev_int = 0;
    return timer_create(CLOCK_MONOTONIC, &sigev, timer);
}

static int activate_timer(timer_t timer, time_t seg, long nsec) {
    struct itimerspec it;
    it.it_interval.tv_sec = seg;
    it.it_interval.tv_nsec = nsec;
    it.it_value.tv_sec = 0;
    it.it_value.tv_nsec = 1;
    return timer_settime(timer, 0, &it, NULL);
}

static inline long generate_random(pthread_mutex_t* mutex, long max, long min) {
    pthread_mutex_lock(mutex);
    long res = min + random() % (max - min + 1);
    pthread_mutex_unlock(mutex);
    return res;
}

#define SIN_BATERIA SIGRTMIN
#define LLAMADA SIGRTMIN + 1
#define SMS SIGRTMIN + 2
#define TIMER_MONITOR SIGRTMIN + 3
#define TIMER_BATERIA SIGRTMIN + 4
#define TIMER_EVENTO SIGRTMIN + 5

int bateria = 100;
sigset_t sigset;

// lo nuestro
typedef struct {
    Evento evento;
    pthread_mutex_t random, mutex;
    pthread_t bateria, monitor, control;
    int periodo_mon, periodo_bateria;
} Telefono;

typedef struct {
    pthreat_t thread;
    int periodo;
} Evento;

void* evento_task(void*);
void* bateria_task(void*);
void* control_task(void*);
void* monitor_task(void*);

int main() {
    // configuracion de senyales
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srandom(time(NULL));
    sigemptyset(&sigset);
    sigaddset(&sigset, SIN_BATERIA);
    sigaddset(&sigset, LLAMADA);
    sigaddset(&sigset, SMS);
    sigaddset(&sigset, TIMER_BATERIA);
    sigaddset(&sigset, TIMER_EVENTO);
    sigaddset(&sigset, TIMER_MONITOR);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    // configuracion de threads
    change_priority(SCHED_FIFO, 40);
    Telefono telf;
    pthread_mutex_init(&telf.random, NULL);
    pthread_mutex_init(&telf.mutex, NULL);
    create_thread(&telf.bateria, bateria_task, &telf, SCHED_FIFO, 29);
    create_thread(&telf.monitor, monitor_task, &telf, SCHED_FIFO, 25);
    create_thread(&telf.control, control_task, &telf, SCHED_FIFO, 25);
    create_thread(&telf.evento.thread, evento_task, &telf, SCHED_FIFO, 28);

    pthread_join(telf.monitor, NULL);
    pthread_join(telf.bateria, NULL);
    pthread_join(telf.control, NULL);
    pthread_join(telf.evento.thread, NULL);
    return 0;
}

void* evento_task(void* argg) {
    Telefono* telf = (Telefono*) argg;
}


void* bateria_task(void* argg) {
    Telefono* telf = (Telefono*) argg;
}


void* control_task(void* argg) {
    Telefono* telf = (Telefono*) argg;
    sigset_t sig_thread;
    sigemptyset(&sig_thread);
    sigaddset(&sig_thread, SMS);
    sigaddset(&sig_thread, LLAMADA);
    sigaddset(&sig_thread, SIN_BATERIA);

    int has_battery = 1;
    while(has_battery) {
        int info;
        sigwait(&sig_thread, &info);
        if(info == LLAMADA) {
            long telf = generate_random(&telf->random, 0, 99);
            printf("[Telefono]: Llamada recibida de %ld\n", telf);
        } else if (info == SMS) {
            long telf = generate_random(&telf->random, 0, 99);
            printf("[Telefono]: SMS recibido de %ld\n", telf);
        } else if (info == SIN_BATERIA) {
            has_battery = 0;
        }
    }
}


void* monitor_task(void* argg) {
    Telefono* telf = (Telefono*) argg;
    sigset_t sig_thread;
    sigemptyset(&sig_thread);
    sigaddset(&sig_thread, TIMER_MONITOR);
    sigaddset(&sig_thread, SIN_BATERIA);

    timer_t timer;

    create_timer(&timer, TIMER_MONITOR);
    activate_timer(timer, telf->periodo_mon, 0);
    int has_battery = 1;
    while(has_battery) {
        int info;
        sigwait(&sig_thread, &info);
        if(info == TIMER_MONITOR) {
            pthread_mutex_lock(&telf->mutex);
            printf("[Monitor]: Bateria restante: %d\n", bateria);
            pthread_mutex_unlock(&telf->mutex);
        } else if (info == SIN_BATERIA) {
            has_battery = 0;
        }
    }
    timer_delete(timer);
}