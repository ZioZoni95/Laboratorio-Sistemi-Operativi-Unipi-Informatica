#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.h"
#include "util.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t isFull = PTHREAD_COND_INITIALIZER;
pthread_cond_t isEmpty = PTHREAD_COND_INITIALIZER;
queue_t Q;
int produzione_terminata = 0;
int somma_totale = 0;

int W, N, k, C;
int *array;

void *T_Master(void *arg);
void *T_Worker(void *arg);

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <Worker> <N> <k> <C>\n", argv[0]);
        return EXIT_FAILURE;
    }

    W = atoi(argv[1]);
    N = atoi(argv[2]);
    k = atoi(argv[3]);
    C = atoi(argv[4]);

    if (W < 1 || N <= 0 || k <= 0 || C <= 0) {
        fprintf(stderr, "Errore: Parametri non validi\n");
        return EXIT_FAILURE;
    }

    array = malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) {
        array[i] = rand() % 100;
    }

    queue_init(&Q, C);

    pthread_t master_thread, workers[W];

    printf("[Main] 🚀 Avvio thread Master e %d Worker...\n", W);

    pthread_create(&master_thread, NULL, T_Master, NULL);
    for (int i = 0; i < W; i++) {
        pthread_create(&workers[i], NULL, T_Worker, NULL);
    }

    pthread_join(master_thread, NULL);
    for (int i = 0; i < W; i++) {
        pthread_join(workers[i], NULL);
    }

    printf("[Main] ✅ Esecuzione completata\n");
    queue_destroy(&Q);
    free(array);
    return EXIT_SUCCESS;
}
