#include<signal.h>
#include<stdio.h>
#include<pthread.h>
#include <sys/types.h>
#include<unistd.h>
#include <stdlib.h>
#include<semaphore.h>

#define  PLAZAS  5  // numero de plazas de aparcamiento
#define  M  8  // numero de coches


sem_t espera, entrada;
sigset_t sigset; // conjunto de señales


void* parking(void* argg) {
    int id = *(int*) argg;
    while(1) {
        sem_wait(&entrada);
        printf("El coche %d esta llegando al parking\n", id);
        kill(getpid(), SIGRTMIN); // mando la senyal minima de tiempo real que podemos mandar. Senyal del SO.
        sem_wait(&espera);
        printf("Aparcando coche %d\n", id);
        sem_post(&entrada);
        sleep(rand() % 2);
        printf("El coche %d esta saliendo del prakign\n", id);
        kill(getpid(), SIGRTMAX); // mando la senyal maxima de tiempo real que podemos mandar. Senyal del SO.
        sleep(2);
    }
    pthread_exit(NULL);
}

void* controlador() {
    int info;
    int cont = 0;
    int esperando = 0;
    while(1) {
        sigwait(&sigset, &info);
        if(info == SIGRTMIN && cont < PLAZAS) {
            sem_post(&espera);
            cont++;
        } else if (info == SIGRTMIN) {
            esperando = 1;
            printf("praking yeno \n");
        }
        if(info == SIGRTMAX) {
            cont--;
            printf("Hay %d coches dentro\n", cont);
            if(esperando) {
                esperando = 0;
                cont++;
                sem_post(&espera);
            }
        }
    }
    pthread_exit(NULL);
} 

int main() {
    srand(time(NULL));
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN);
    sigaddset(&sigset, SIGRTMAX);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    
    sem_init(&espera,0,0);
    sem_init(&entrada,0,1);
    pthread_t coche[M], controlador_t;
    int id[M];
    int i;
    for(i=0;i<M;i++){
        id[i]=i;
        pthread_create(&coche[i], NULL, parking, &id[i]);
    }

    pthread_create(&controlador_t,NULL, controlador, NULL);
    
    for(i=0;i<M;i++){
        pthread_join(coche[i],NULL);
    }
    
    pthread_join(controlador_t,NULL);

    return 0;
}