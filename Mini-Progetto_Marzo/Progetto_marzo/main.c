#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "threads.h"

/*
 * Funzione main:
 * 1. Legge i parametri da linea di comando:
 *       <num_worker> <N> <k> <capienza_coda>
 *    - num_worker: numero di thread Worker (minimo 1)
 *    - N: dimensione dell'array con gli elementi da sommare
 *    - k: numero massimo di coppie per gruppo (parametro del Master)
 *    - capienza_coda: capacità massima della coda Q (C)
 *
 * 2. Inizializza l'array (con valori da 1 a N) e la coda.
 * 3. Crea il thread Master e i thread Worker.
 * 4. Attende la terminazione di tutti i thread.
 * 5. Libera le risorse allocate e chiude correttamente mutex e variabili condizione.
 */
int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <num_worker> <N> <k> <capienza_coda>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    num_workers = atoi(argv[1]);
    N = atoi(argv[2]);
    k = atoi(argv[3]);
    capacity = atoi(argv[4]);
    
    if (num_workers < 1 || N < 1 || k < 1 || capacity < 1) {
        fprintf(stderr, "Parametri non validi\n");
        exit(EXIT_FAILURE);
    }
    
    // Inizializzazione dell'array da sommare (i valori sono da 1 a N)
    array = malloc(N * sizeof(int));
    if (!array) {
        perror("malloc array");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < N; i++) {
        array[i] = i + 1;
    }
    
    // Inizializzazione della coda Q con la capacità specificata (C)
    queue_init(&Q, capacity);
    
    pthread_t master_tid;
    pthread_t *worker_tids = malloc(num_workers * sizeof(pthread_t));
    if (!worker_tids) {
        perror("malloc worker_tids");
        exit(EXIT_FAILURE);
    }
    
    // Creazione del thread Master
    if (pthread_create(&master_tid, NULL, thread_master, NULL) != 0) {
        perror("pthread_create master");
        exit(EXIT_FAILURE);
    }
    
    // Creazione dei thread Worker, ciascuno con un proprio worker_id tramite WorkerArgs
    for (int i = 0; i < num_workers; i++) {
        WorkerArgs *args = malloc(sizeof(WorkerArgs));
        if (!args) {
            perror("malloc WorkerArgs");
            exit(EXIT_FAILURE);
        }
        args->worker_id = i;
        if (pthread_create(&worker_tids[i], NULL, thread_worker, (void *)args) != 0) {
            perror("pthread_create worker");
            exit(EXIT_FAILURE);
        }
    }
    
    // Attesa del completamento del thread Master e dei Worker
    pthread_join(master_tid, NULL);
    for (int i = 0; i < num_workers; i++) {
        pthread_join(worker_tids[i], NULL);
    }
    
    // Pulizia: distruzione della coda e rilascio delle risorse allocate
    queue_destroy(&Q);
    free(worker_tids);
    free(array);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);
    pthread_cond_destroy(&cond_results);
    
    return 0;
}
