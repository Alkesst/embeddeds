#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#define NUM_THREADS 5


void* hello_world(void* threadid) {
    // *(long*) threadid casts void* threadid to long* threadid and then we access the long* threadid using *
    // to get the value of the pointer. 
    printf("Yes, i'm the %ld thread!\n", *(long*) threadid);
    return 0;
}

int main() {
    pthread_t threads[NUM_THREADS];
    long idThread[NUM_THREADS];
    int returnCode; 

    for(long t = 0; t < NUM_THREADS; t++) {
        idThread[t] = t;
        printf("creating the %ld thread\n", t);
        returnCode = pthread_create(& threads[t], NULL, hello_world, & idThread[t]);
        if(returnCode) {
            printf("ERROR; return code from pthread_create is %d", returnCode);
            exit(-1);
        }
    }

    for(long t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }
    
    return 0;
}
