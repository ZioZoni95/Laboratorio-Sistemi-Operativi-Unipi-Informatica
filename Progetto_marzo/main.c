#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "threads.h"

/*
   Funzione main:
   - Legge i parametri da linea di comando: numero di worker, dimensione dell'array, k, capacità della coda.
   - Alloca e inizializza l'array.
   - Inizializza la coda.
   - Crea il thread master e i thread worker.
   - Attende la terminazione di tutti i thread.
   - Pulisce le risorse allocate.
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

    // Allocazione e inizializzazione dell'array da sommare (valori da 1 a N)
    array = malloc(N * sizeof(int));
    if (!array) {
        perror("malloc array");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < N; i++) {
        array[i] = i + 1;
    }

    // Inizializzazione della coda con la capacità specificata
    queue_init(&Q, capacity);

    pthread_t master_tid;
    pthread_t *worker_tids = malloc(num_workers * sizeof(pthread_t));
    if (!worker_tids) {
        perror("malloc worker_tids");
        exit(EXIT_FAILURE);
    }

    // Creazione del thread MASTER
    if (pthread_create(&master_tid, NULL, thread_master, NULL) != 0) {
        perror("pthread_create master");
        exit(EXIT_FAILURE);
    }

    // Creazione dei thread WORKER, ciascuno con i propri parametri
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

    // Attesa della terminazione del thread MASTER e dei WORKER
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
