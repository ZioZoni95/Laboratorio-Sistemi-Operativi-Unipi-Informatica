//sync.c - Implementazione delle funzioni di sincronizzazione
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sync.h>

//Inizializzazione delle variabili globali
sem_t mux, waitT1T2, waitT3T4;
int T1T2 = 0, T1T2waiting = 0, T3T4 = 0, T3T4waiting = 0;

void accediT1T2() {
    bool ok = false;

    sem_wait(&mux);
    if (T3T4 == 0 && T1T2 == 0) {  
        T1T2 = 1;  
        ok = true;
        printf("Scrittore %ld ENTRA in sezione critica.\n", pthread_self());
    } else {  
        T1T2waiting++;  
        printf("Scrittore %ld in ATTESA (lettori o scrittori attivi).\n", pthread_self());
    }
    sem_post(&mux);  

    if (!ok) {  
        sem_wait(&waitT1T2);
        printf("Scrittore %ld RIPRENDE l'esecuzione.\n", pthread_self());
    }
}


void rilasciaT1T2() {
    sem_wait(&mux);
    printf("Scrittore %ld LASCIA la sezione critica.\n", pthread_self());
    
    T1T2 = 0;
    if (T1T2waiting > 0) {  
        T1T2 = 1;
        T1T2waiting--;
        printf("Scrittore in attesa VIENE SVEGLIATO.\n");
        sem_post(&waitT1T2);
    } else {  
        while (T3T4waiting > 0) {  
            T3T4waiting--;
            printf("Lettore in attesa VIENE SVEGLIATO.\n");
            sem_post(&waitT3T4);
        }
    }
    sem_post(&mux);
}

   


void accediT3T4() {
    bool ok = false;

    sem_wait(&mux);
    if (T1T2waiting == 0 && T1T2 == 0) {  
        T3T4++;
        ok = true;
        printf("Lettore %ld ENTRA in sezione critica.\n", pthread_self());
    } else {  
        T3T4waiting++;
        printf("Lettore %ld in ATTESA (scrittore attivo o in attesa).\n", pthread_self());
    }
    sem_post(&mux);

    if (!ok) {  
        sem_wait(&waitT3T4);
        printf("Lettore %ld RIPRENDE l'esecuzione.\n", pthread_self());
    }
}


void rilasciaT3T4() {
    sem_wait(&mux);
    printf("Lettore %ld LASCIA la sezione critica.\n", pthread_self());

    T3T4--;
    if (T3T4 == 0 && T1T2waiting > 0) {  
        T1T2 = 1;
        T1T2waiting--;
        printf("Scrittore in attesa VIENE SVEGLIATO.\n");
        sem_post(&waitT1T2);
    }
    sem_post(&mux);
}
