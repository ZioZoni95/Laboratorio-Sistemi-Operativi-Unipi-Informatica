/*
Esercizio 1 (buffer con capacità limitata)
Scrivere un programma C con due threads, un produttore (P) ed un consumatore (C).
Il thread P genera, uno alla volta, una sequenza di numeri inserendoli in un buffer 
di una sola posizione condiviso con il thread C.
Il thread consumatore estrae i numeri dal buffer e li stampa sullo standard output.
Se il buffer e' pieno P attende che C consumi il dato, analogamente se il buffer e' vuoto C attende che P produca un valore da consumare.
*/

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "wrappers.h"

#define NUM_ITEMS 10 //Numero di elementi da produrre

//Buffer Condiviso
int buffer[1];
bool empty = true; //Flag per indicare se il buffer è vuoto

//Mutex e variabili di condizione
pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//Funzione del produttore
int produci(){
    return rand() % 100; //Genera un numero casuale
}

static void *produttore(void *arg){
    for(int i = 0; i < NUM_ITEMS; i++){
        Pthread_mutex_lock(&mux);

        //Attende se il buffer è pieno
        while(!empty){
            pthread_cond_wait(&cond, &mux);
        }

        //Produce un valore
        buffer[0] = produci();
        empty = false;
        printf("Produttore ha prodotto: %d\n", buffer[0]);

        //Segnala al consumatore che  può consumare
        pthread_cond_signal(&cond);
        Pthread_mutex_unlock(&mux);

        sleep(1); //Simula il tempo di produzione
    }
    return NULL;
}

//Funzione del consumatore
static void *consumatore(void *arg){
    for(int i = 0; i< NUM_ITEMS; i++){
        Pthread_mutex_lock(&mux);

        //Attende se il buffer è vuoto
        while(empty){
            pthread_cond_wait(&cond, &mux);
        }

        //Consuma il valore
        printf("Consumatore ha consumato: %d\n", buffer[0]);
        empty = true;

        //Segnala al produttore che può produrre
        pthread_cond_signal(&cond);
        Pthread_mutex_unlock(&mux);

        sleep(1); //Simula il tempo di consumo
    }
    return NULL;
}

int main(){
    pthread_t thr_produttore, thr_consumatore;
    
    srand(time(0));

    //Creazione dei thread
    Pthread_create(&thr_produttore, NULL, produttore, NULL);
    Pthread_create(&thr_consumatore, NULL, consumatore, NULL);

    //Attnede la terminazione dei thread
    pthread_join(thr_produttore, NULL);
    pthread_join(thr_consumatore, NULL);

    printf("Terminato!\n");
    return 0;
}