#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t pueden_pasar_horizontal, pueden_pasar_vertical;
int semaforo_vertical_verde, semaforo_horizontal_verde;


void* controlador() {
    semaforo_vertical_verde = 0;
    semaforo_horizontal_verde = 1;
    while(1) {
        semaforo_horizontal_verde = !semaforo_horizontal_verde;
        if(semaforo_horizontal_verde) {
            printf("Semaforo horizontal en verde\n");
            pthread_cond_broadcast(&pueden_pasar_horizontal);
        } else {
            printf("Semaforo horizontal en rojo\n");
        }
        semaforo_vertical_verde = !semaforo_vertical_verde;
        if(semaforo_vertical_verde) {
            printf("Semaforo vertical en verde\n");
            pthread_cond_broadcast(&pueden_pasar_vertical);
        } else {
            printf("Semaforo vertical en rojo\n");
        }
        sleep(10);   
    }
}

void* coches_vertical(void* argg) {
    int id = *(int*) argg;
    while(1) {
        pthread_mutex_lock(&mutex);
        while(!semaforo_vertical_verde) {
            printf("Coche %d esperando en el semaforo vertical\n", id);
            pthread_cond_wait(&pueden_pasar_vertical, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        printf("Coche %d pasando por el semaforo vertical\n", id);
        sleep(rand() % 4 + 1);
    }
    return 0;
}

void* coches_horizontal(void* argg) {
    int id = *(int*) argg;
    while(1) {
        pthread_mutex_lock(&mutex);
        while(!semaforo_horizontal_verde) {
            printf("Coche %d esperando en el semaforo horizontal\n", id);
            pthread_cond_wait(&pueden_pasar_horizontal, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        printf("Coche %d pasando por el semaforo horizontal\n", id);
        sleep(rand() % 4 + 1);
    }
    return 0;
}



int main() {
    srand(time(NULL));
    pthread_t coches_v[5], coches_h[5], controller;
    int id_v[5], id_h[5];
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&pueden_pasar_horizontal, NULL);
    pthread_cond_init(&pueden_pasar_vertical, NULL);

    pthread_create(&controller, NULL, controlador, NULL);

    for(int i = 0; i < 5; i++) {
        id_v[i] = i;
        id_h[i] = i + 5;
        pthread_create(&coches_v[i], NULL, coches_vertical, &id_v[i]);
        pthread_create(&coches_h[i], NULL, coches_horizontal, &id_h[i]);
    }

    pthread_join(controller, NULL);

    for(int i = 0; i < 5; i++) {
        pthread_join(coches_v[i], NULL);
        pthread_join(coches_h[i], NULL);
    }

    return 0;
}


