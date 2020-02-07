#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#define THREADS 10
#define CANS 5
#define VECES 5

sem_t cliente_usando;
sem_t reponiendo;
pthread_mutex_t mutex;

int disponibles = CANS;

void* obtener_lata(void* argg) {
    int id = *(int*) argg, i = 0;

    printf("Soy el customer %d init", id);

    while(i < VECES) {
        pthread_mutex_lock(&mutex);

        if(sem_trywait(&cliente_usando) >= 0) {
            printf("Cliente %d usando maquina", id);
        }

    }
}