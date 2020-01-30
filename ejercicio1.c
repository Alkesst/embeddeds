#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#define NUM_THREADS 3

typedef struct {
    long threadid;
    char character;
    int times;
} ThreadHandlerParameters;

void* print_handler(void* thread_parameters) {
    ThreadHandlerParameters params = *(ThreadHandlerParameters*) thread_parameters;
    for(int i = 0; i < params.times; i++) {
        printf("I'm the Thread %ld and I'm going to print %c (%d times)!!!!\n", params.threadid, params.character, i + 1);
    }
    return 0;
}


int main(int argc, char const *argv[]) {
    pthread_t threads[NUM_THREADS];
    long idThread[NUM_THREADS];
    pthread_attr_t attr;
    int returnCode; 
    ThreadHandlerParameters params[NUM_THREADS];

    if(argc < 7) {
        printf("PLEASE PROVIDE 6 ARGS");
        exit(-1);
    }

    pthread_attr_init(&attr);

    for(long t = 0; t < NUM_THREADS; t++) {
        idThread[t] = t;
        // big brain time
        // It's required to have an array of parameters, because when the iteration finishes, the param variable
        // is removed and implies that param is no longer available. We don't know when a thread will access the
        // data, so better if we put that "shared memory" in a hogher scope.
        printf("Creating thread with id %ld\n", t);
        ThreadHandlerParameters param = { idThread[t], *argv[2*t + 1] , atoi(argv[2*t + 2])};
        params[t] = param;
        returnCode = pthread_create(& threads[t], &attr, print_handler, & params[t]);
        if(returnCode) {
            printf("Error with pthread_create %d\n", returnCode);
            exit(-1);
        }
    }

    for(long t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    return 0;
}
