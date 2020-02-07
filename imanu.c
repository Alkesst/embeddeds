#include <pthread.h>
#include <signal.h>
#include <stdio.h>

void ManejadorSig(int signo, siginfo_t* info, void* context) {
    printf(context, signo);
    printf("Soy el manejador de la senal #%d Valor: %d ", info->si_signo, info->si_value.sival_int);
    printf("Code: (%d) ", info->si_code);

    if(info->si_code == SI_USER) {
        printf("SI_USER \n");
    } else if (info->si_code == SI_TIMER) {
        printf("SI_TIMER \n");
    } else if (info->si_code == SI_QUEUE) {
        printf("SI_QUEUE \n");
    } else if (info->si_code == SI_ASYNCIO) {
        printf("SI_ASYNCIO \n");
    } else if (info->si_code == SI_MESGQ) {
        printf("SI_MESGQ \n");
    }
}

int main() {
    sigset_t sigset;
    struct sigaction act;
    
    act.sa_sigaction = ManejadorSig;
    sigemptyset(&act.sa_mask);

    act.sa_flags = SA_SIGINFO | SA_RESTART;

    if(sigaction(SIGRTMAX, &act, NULL) < 0) {
        perror("Sigaction fallado\n");
    }

    sigemptyset(&sigset);
    while(1) {

    }
}