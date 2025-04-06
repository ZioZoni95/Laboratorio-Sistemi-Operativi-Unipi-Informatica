#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>
#include "queue.h"

/*
   Struttura WorkerArgs:
   Utilizzata per passare parametri specifici ad ogni thread worker.
   Attualmente contiene solo l'identificativo del worker.
*/
typedef struct {
    int worker_id;
} WorkerArgs;

/*
   Dichiarazione delle variabili condivise:
   Queste variabili vengono definite nel file threads.c e sono accessibili (tramite extern)
   agli altri file che includono questo header.
*/
extern Queue Q;                        // La coda condivisa
extern pthread_mutex_t mutex;          // Mutex per la sincronizzazione della coda
extern pthread_cond_t cond_not_full;   // Variabile condizione per la coda piena
extern pthread_cond_t cond_not_empty;  // Variabile condizione per la coda vuota
extern pthread_cond_t cond_results;    // Variabile condizione per la raccolta dei risultati
extern int production_finished;        // Flag che indica la fine della produzione da parte del master
extern int results_count;              // Contatore dei risultati parziali ricevuti
extern int *array;                     // L'array di interi da sommare
extern int N, k, num_workers, capacity; // Parametri di configurazione: dimensione dell'array, k, numero di worker, capacità della coda

/*
   Dichiarazione delle funzioni dei thread:
   - thread_master: Funzione eseguita dal thread master.
   - thread_worker: Funzione eseguita dai thread worker.
*/
void* thread_master(void *arg);
void* thread_worker(void *arg);

#endif
