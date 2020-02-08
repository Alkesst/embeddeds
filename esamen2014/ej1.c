#include <pthread.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#define ENTRADA SIGRTMIN
#define SALIDA SIGRTMAX
#define TIMER_TORNO1 SIGRTMIN + 1
#define TIMER_TORNO2 SIGRTMIN + 2

static inline long random_value(pthread_mutex_t* mutex, long min, long max) {
    pthread_mutex_lock(mutex);
    long r = min + random() % (max - min + 1);
    pthread_mutex_unlock(mutex);
    return r;
}
// crea el timer;
static int create_timer(timer_t* timer, int signo) {
    struct sigevent sigev;
    // las notificaciones del timer van a llegar a traves de senyales;
    sigev.sigev_notify = SIGEV_SIGNAL;
    // cual es el numero de senyal que voy a recibir en la notificacion;
    sigev.sigev_signo = signo;
    // No sirve pa na
    sigev.sigev_value.sival_int = 0;
    // usamos el clock monotonic. e inicializamos el timer con la config de sigev;
    return timer_create(CLOCK_MONOTONIC, &sigev, timer);
}

// se activan los timers; 
static int activate_timer(timer_t timer, time_t seg, long nsec) {
    struct itimerspec it;
    // configuramos el intervalo en segundos del timer;
    it.it_interval.tv_sec = seg;
    // lo mismo pero en nanosecs
    it.it_interval.tv_nsec = nsec;
    // si no se pone asi peta
    it.it_value.tv_sec = 0;
    it.it_value.tv_nsec = 1;
    return timer_settime(timer, 0, &it, NULL);
}


int personas;
typedef struct {
    pthread_mutex_t mutex;
    pthread_mutex_t* random;
    pthread_t torno1, torno2, controlador;
    int periodo_entrada, periodo_salida, prob_entrada, prob_salida;
} Controlador;

sigset_t sigset;


void* torno_entrada();
void* torno_salida();
void* controlador_task();

int main() {
    srandom(time(NULL));
    Controlador controlador;
    pthread_mutex_init(&controlador.mutex, NULL);
    controlador.periodo_entrada = 1;
    controlador.periodo_salida = 2;
    controlador.prob_entrada = 60;
    controlador.prob_salida = 40;
    sigemptyset(&sigset);
    sigaddset(&sigset, ENTRADA);
    sigaddset(&sigset, SALIDA);
    sigaddset(&sigset, TIMER_TORNO1);
    sigaddset(&sigset, TIMER_TORNO2);

    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    pthread_create(&controlador.controlador, NULL, controlador_task, &controlador);
    pthread_create(&controlador.torno1, NULL, torno_entrada, &controlador);
    pthread_create(&controlador.torno2, NULL, torno_salida, &controlador);

    pthread_join(controlador.torno1, NULL);
    pthread_join(controlador.torno2, NULL);
    pthread_join(controlador.controlador, NULL);

    return 0;
}

void* torno_entrada(void* contr) {
    Controlador* controlador = (Controlador*) contr;
    // struct timespec next;
    // clock_gettime(CLOCK_MONOTONIC, &next);
    // next.tv_sec += controlador->periodo_entrada;
    const union sigval useless = { 0 };
    sigset_t sig_thread;
    sigemptyset(&sig_thread);
    sigaddset(&sig_thread, TIMER_TORNO1);

    timer_t timer;

    printf("el timer entrada %d\n", create_timer(&timer, TIMER_TORNO1));
    printf("el timer entrada %d activado\n", activate_timer(timer, controlador->periodo_entrada, 0));
    printf("YEET entrada\n");
    while(1) {
        // CLOCK_MONOTONIC el tipo de reloj, TIMER_ABSOLUTE TIME "ESPERA HASTA QUE LLEGUE X TIEMPO", QUE HA FALLADO POR UNA INTERRUPCION (EINTR);
        // while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);
        // esto si usaramos el nanosleep
        int signo;
        printf("Espero entrada\n");
        sigwait(&sig_thread, &signo);
        printf("He pasado entrada\n");

        if(signo == TIMER_TORNO1) {
            printf("Recibi senyal entrada\n");
            pthread_mutex_lock(&controlador->mutex);
            long random = random_value(controlador->random, 0, controlador->prob_entrada);
            if(random < controlador->prob_entrada) {
            // sigqueue(pid, senyal, valor inutil) sirve para mandar de verdad una senyal.
                sigqueue(getpid(), ENTRADA, useless);
                printf("Entro\n");
            }
            pthread_mutex_unlock(&controlador->mutex);
        }
        // si usaramos nanoslpeep y no timers;
        // next.tv_sec += controlador->periodo_entrada;
    }
    timer_delete(timer);
}

void* torno_salida(void* contr) {
    Controlador* controlador = (Controlador*) contr;
    // struct timespec next;
    // clock_gettime(CLOCK_MONOTONIC, &next);
    // next.tv_sec += controlador->periodo_salida;
    const union sigval useless = { 0 };
    sigset_t sig_thread;
    sigemptyset(&sig_thread);
    sigaddset(&sig_thread, TIMER_TORNO2);

    timer_t timer;
    

    create_timer(&timer, TIMER_TORNO2);
    activate_timer(timer, controlador->periodo_salida, 0);
    printf("YEET salida\n");
    while(1) {
        // CLOCK_MONOTONIC el tipo de reloj, TIMER_ABSOLUTE TIME "ESPERA HASTA QUE LLEGUE X TIEMPO", QUE HA FALLADO POR UNA INTERRUPCION (EINTR);
        // while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR);

        int signo;
        printf("Espero salida\n");
        sigwait(&sig_thread, &signo);
        printf("He pasado salida\n");
        if(signo == TIMER_TORNO2) {
            printf("Recibi senyal salida\n");
            pthread_mutex_lock(&controlador->mutex);
            long random = random_value(controlador->random, 0, controlador->prob_salida);
            if(random < controlador->prob_salida) {
                // sigqueue(pid, senyal, valor inutil) sirve para mandar de verdad una senyal.
                sigqueue(getpid(), SALIDA, useless);
            }
            pthread_mutex_unlock(&controlador->mutex);
        }

        // next.tv_sec += controlador->periodo_salida;
    }
    timer_delete(timer);
}

void* controlador_task(void* contr) {
    Controlador* controlador = (Controlador*) contr;
    int signo;
    while(true) {
        sigwait(&sigset, &signo);
        if(signo == ENTRADA) {
            pthread_mutex_lock(&controlador->mutex);
            personas++;
            printf("Ha entrado una persona. Hay %d personas dentro\n", personas);
            pthread_mutex_unlock(&controlador->mutex);
        }
        if(signo == SALIDA) {
            pthread_mutex_lock(&controlador->mutex);
            personas--;
            printf("Ha salido una persona. Hay %d personas dentro\n", personas);
            pthread_mutex_unlock(&controlador->mutex);
        }
    }
    return 0;
}