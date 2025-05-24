#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "threads_types.h"
#include "queue.h"
#include "master.h"
#include "worker.h"

/* Variabili globali per le code e la sincronizzazione */
Queue_t *workQueue;
Queue_t *resultQueue;

pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
int finished = 0;

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <num_worker> <dimensione_array> <k> <capacita_coda>\\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    Parameters params;
    params.numWorkers = atoi(argv[1]);
    params.N = atoi(argv[2]);
    params.k = atoi(argv[3]);
    params.C = atoi(argv[4]);
    
    if (params.k > MAX_K) {
        fprintf(stderr, "Errore: k troppo grande, massimo %d\n", MAX_K);
        return EXIT_FAILURE;
    }
    
    params.array = malloc(params.N * sizeof(int));
    if (!params.array) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < params.N; i++) {
        params.array[i] = 1;
    }
    
    workQueue = initQueue(params.C);
    resultQueue = initQueue(params.C);
    if (!workQueue || !resultQueue) {
        fprintf(stderr, "Errore nell'inizializzazione delle code\n");
        return EXIT_FAILURE;
    }
    
    pthread_t master;
    pthread_t *workers = malloc(params.numWorkers * sizeof(pthread_t));
    WorkerArg *wargs = malloc(params.numWorkers * sizeof(WorkerArg));
    if (!workers || !wargs) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    
    /* Creazione del thread Master */
    if (pthread_create(&master, NULL, master_thread, &params) != 0) {
        perror("pthread_create (master)");
        return EXIT_FAILURE;
    }
    
    /* Creazione dei thread Worker, usando una struttura per passare il worker_id */
    for (int i = 0; i < params.numWorkers; i++) {
        wargs[i].params = &params;
        wargs[i].worker_id = i;
        if (pthread_create(&workers[i], NULL, worker_thread, &wargs[i]) != 0) {
            perror("pthread_create (worker)");
            return EXIT_FAILURE;
        }
    }
    
    pthread_join(master, NULL);
    for (int i = 0; i < params.numWorkers; i++) {
        pthread_join(workers[i], NULL);
    }
    
    deleteQueue(workQueue);
    deleteQueue(resultQueue);
    free(params.array);
    free(workers);
    free(wargs);
    
    return EXIT_SUCCESS;
}
