#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#define N 10
#define VECES 2
#define MAXDELAY 4

sem_t quiero_entrar[2];
pthread_mutex_t wc = PTHREAD_MUTEX_INITIALIZER;

int personasDentro[2] = {0, 0};
int turno = -1;
char* sexos[2] = {"el chico", "la chica"};

void delay() {
    sleep(rand() % MAXDELAY);
}

void* Persona(void* arg) {
    int id = *(int*) arg;
    int sexo = rand() % 2;
    int otroSexo = (sexo + 1) % 2;
    int i = 0;
    int dentro = 0;
    printf("Soy %s %d inicializado\n", sexos[sexo], id);

    while(i < VECES) {
        pthread_mutex_lock(&wc);
        if(turno < 1) {
            turno = sexo;
        }

        if(!personasDentro[otroSexo] && turno == sexo) {
            if(sem_trywait(&quiero_entrar[sexo]) >= 0) {
                personasDentro[sexo]++;
                printf("Soy %s %d entrando (chicos %d) (Chicas %d)\n", sexos[sexo], id, personasDentro[0], personasDentro[1]);
                dentro = 1;
                i++;
            }
        }

        pthread_mutex_unlock(&wc);

        if(dentro) {
            sleep(1);

            sem_post(&quiero_entrar[sexo]);
            pthread_mutex_lock(&wc);

            personasDentro[sexo]--;
			printf("Soy %s %d saliendo (chicos: %d) (chicas %d)\n", sexos[sexo], id, personasDentro[0], personasDentro[1]);			
			dentro = 0;
			if (personasDentro[sexo] <= 0 && turno == sexo) {
				turno = -1;
			}
            pthread_mutex_unlock(&wc);
        }
    }

    return 0;
}

int main() {
    pthread_t personas[N];
    pthread_attr_t attr;
    int ids[N], i;
    srand(time(NULL));
    pthread_attr_init(&attr);
    sem_init(&quiero_entrar[0], 0, 5);
    sem_init(&quiero_entrar[1], 0, 1);

    for (i = 0; i < N; i++) {
		ids[i] = i;
		pthread_create(&personas[i], &attr, Persona, &ids[i]);
	}

	for (i = 0; i < N; i++) {
		pthread_join(personas[i], NULL);
	}
}