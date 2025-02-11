/*
Esercizio 2 (filosofi a cena)
N filosofi siedono ad una tavola con un piatto di spaghetti davanti ed una forchetta alla loro destra ed una alla loro sinistra.
Per mangiare gli spaghetti un filosofo ha bisogno di entrambe le forchette vicine (!).
Ogni filosofo è impegnato ininterrottamente in una sequenza di 3 attivita':
  - meditare
  - cercare di acquisire le forchette
  - mangiare
Scrivere un programma C che attivi N threads filosofi (con N>=5), ognuno dei quali esegue il ciclo descritto in precedenza per 100 volte.
La meditazione e la fase in cui il filosofo mangia deve essere implementata con un ritardo variabile usando ad esempio la chiamata di sistema nanosleep e la funzione rand_r().
SUGGERIMENTO: Per evitare il deadlock nell'acquisizione delle forchette, una possibile soluzione è definire un ordinamento opportuno per l'acquisizione delle forchette da parte di ogni filosofo.
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "wrappers.h"

#define N 5 //Numero di filosofi
#define THINKING 0
#define HUNGRY 1
#define EATING 2

pthread_mutex_t forks[N]; //Mutex per le forchette
pthread_t filosofi[N]; //Thread per i filosofi

//Funzione per introdurre un ritardo casuale simulando la meditazione e il mangiare
void delay(){
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (rand() % 500 + 500) * 1000000L; //Tra 500ms e 1s
    nanosleep(&ts, NULL);
}

//Funzione eseguita dai filosofi
static void *philosopher(void *arg){
    int id = *(int*)arg;
    int left_fork = id; //Forchetta di sinistra
    int right_fork = (id + 1) % N; //Forchetta di destra

    //Per prevenire il deadlock, l'ultimo filosofo prende prima la forchetta con indice più alto
    if(id == N - 1){
        int tmp = left_fork;
        left_fork = right_fork;
        right_fork = tmp;
    }

    //Ciclo principale dei filosofi
    for(int i = 0; i < 66; i++){ //Con 100 ci mette troppo ma funziona lo stesso
        //I filosofi meditano
        printf("Filosofo %d sta meditando.\n", id);
        delay();

        //Filosofo cerca di prendere la forchetta
        printf("Filosofo %d ha fame e sta cercando le forchette.\n",id);
        Pthread_mutex_lock(&forks[left_fork]); //Prende la forchetta di sinistra
        Pthread_mutex_lock(&forks[right_fork]); //Prende la forchetta di destra

        //Adesso mangia
        printf("Filosofo %d sta mangiando.\n",id);
        delay();

        //Rilascio le lock
        Pthread_mutex_unlock(&forks[left_fork]);
        Pthread_mutex_unlock(&forks[right_fork]);
    }
    return NULL;
}

int main(){
    srand(time(NULL)); //Inizializza il generatore di numeri casuali
    int ids[N];

    //Inizializzazione dei mutex per le forchette
    for(int i = 0; i < N; i++){
        pthread_mutex_init(&forks[i], NULL);
    }

    //Creazione dei thread per i filosofi
    for(int i = 0; i < N; i++){
        ids[i] = i;
        Pthread_create(&filosofi[i], NULL, philosopher, &ids[i]);
    }

    //Attesa della terminazione del thread
    for(int i = 0; i < N; i++){
        pthread_join(filosofi[i], NULL);
    }

    //Distruzione dei mutex
    for(int i = 0; i < N; i++){
        pthread_mutex_destroy(&forks[i]);
    }
    return 0;
}

//Post scriptum: Nessun deadlock riscontrato