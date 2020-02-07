#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>


#define COCHE SIGRTMIN
#define MONOVOL SIGRTMIN + 1
#define SALE_COCHE SIGRTMAX
#define SALE_MONOVOL SIGRTMAX - 1
#define PLAZAS 5
#define COCHES 5
#define MONOVOLS 3

sem_t espera, entrada;
sigset_t sigset;


void* parking_coche(void* argg) {
    int id = *(int*) argg;
    while(1) {
        sem_wait(&entrada);
        printf("El coche %d esta llegando al parking\n", id);
        kill(getpid(), COCHE); // mando la senyal minima de tiempo real que podemos mandar. Senyal del SO.
        sem_wait(&espera);
        printf("Aparcando coche %d\n", id);
        sem_post(&entrada);
        sleep(rand() % 2);
        printf("El coche %d esta saliendo del prakign\n", id);
        kill(getpid(), SALE_COCHE); // mando la senyal maxima de tiempo real que podemos mandar. Senyal del SO.
        sleep(2);
    }
    pthread_exit(NULL);
}

void* parking_monovol(void* argg) {
    int id = *(int*) argg;
    while(1) {
        sem_wait(&entrada);
        printf("El monovolumen %d esta llegando al parking\n", id);
        kill(getpid(), MONOVOL); // mando la senyal minima de tiempo real que podemos mandar. Senyal del SO.
        sem_wait(&espera);
        printf("Aparcando monovolumen %d\n", id);
        sem_post(&entrada);
        sleep(rand() % 4);
        printf("El monovolumen %d esta saliendo del prakign\n", id);
        kill(getpid(), MONOVOL); // mando la senyal maxima de tiempo real que podemos mandar. Senyal del SO.
        sleep(2);
    }
    pthread_exit(NULL);
}

void* controlador() {
    int monovols = 0, coches = 0;
    int esperando[2] = {0, 0};
    int info = 0;

    while(1) {
        sigwait(&sigset, &info);
        if(info == COCHE && (coches + 2*monovols) < PLAZAS) {
            sem_post(&espera);
            coches++;
        } else if (info == MONOVOL && (coches + 2*monovols) < PLAZAS) {
            sem_post(&espera);
            monovols++;
        } else if (info == COCHE) {
            esperando[0] = 1;
            printf("praking yeno \n");
        } else if (info == MONOVOL) {
            esperando[1] = 1;
            printf("praking yeno \n");
        }

        if(info == SALE_COCHE) {
            coches--;
            if(esperando[0]) {
                esperando[0] = 0;
                coches++;
                sem_post(&espera);
            }
        } else if (info == SALE_MONOVOL) {
            monovols--;
            if(esperando[1]) {
                esperando[1] = 0;
                monovols++;
                sem_post(&espera);
            }
        }
    }

    pthread_exit(NULL);
}


int main() {
    srand(time(NULL));
    sigemptyset(&sigset);
    sigaddset(&sigset, COCHE);
    sigaddset(&sigset, SALE_COCHE);
    sigaddset(&sigset, MONOVOL);
    sigaddset(&sigset, SALE_MONOVOL);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    sem_init(&espera,0,0);
    sem_init(&entrada,0,1);

    pthread_t cochethread[COCHES], controlador_t, monovolsthread[MONOVOLS];
    int id_c[COCHES];
    int id_m[COCHES];
    int i;
    for(i=0;i<COCHES;i++){
        id_c[i]=i;
        pthread_create(&cochethread[i], NULL, parking_coche, &id_c[i]);
    }

    for(i=0;i<MONOVOLS;i++){
        id_m[i]=i+COCHES;
        pthread_create(&cochethread[i], NULL, parking_monovol, &id_m[i]);
    }

    pthread_create(&controlador_t,NULL, controlador, NULL);
    
    for(i=0;i<COCHES;i++){
        pthread_join(cochethread[i],NULL);
    }

    for(i=0;i<MONOVOLS;i++){
        pthread_join(monovolsthread[i],NULL);
    }
    
    pthread_join(controlador_t,NULL);
}