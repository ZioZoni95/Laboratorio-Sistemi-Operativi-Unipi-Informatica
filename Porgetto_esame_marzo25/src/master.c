#include <stdio.h>
#include <pthread.h>
#include "queue.h"
#include "util.h"

extern pthread_mutex_t mutex;
extern pthread_cond_t isFull;
extern pthread_cond_t isEmpty;
extern queue_t Q;
extern int produzione_terminata;
extern int somma_totale;
extern int *array, N, k, C;

void *T_Master(void *arg) {
    int num_gruppi = N / (2 * k);
    printf("[Master] 🔵 Produzione iniziata...\n");

    for (int i = 0; i < num_gruppi; i++) {
        pthread_mutex_lock(&mutex);
        while (queue_size(&Q) + (2 * k) > C) {
            pthread_cond_wait(&isFull, &mutex);
        }

        int gruppo[2 * k];
        for (int j = 0; j < 2 * k; j++) {
            gruppo[j] = array[i * (2 * k) + j];
            queue_push(&Q, gruppo[j]);
        }

        printf("[Master] ✅ Inserito gruppo di %d elementi nella coda\n", 2 * k);
        pthread_cond_broadcast(&isEmpty);
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    produzione_terminata = 1;
    pthread_cond_broadcast(&isEmpty);
    pthread_mutex_unlock(&mutex);

    printf("[Master] ✅ Produzione terminata. Aspetto i Worker...\n");

    while (queue_size(&Q) > 0) {
        pthread_mutex_lock(&mutex);
        while (queue_size(&Q) == 0) {
            pthread_cond_wait(&isEmpty, &mutex);
        }
        somma_totale += queue_pop(&Q);
        pthread_mutex_unlock(&mutex);
    }

    printf("[Master] ✅ Somma totale calcolata: %d\n", somma_totale);
    return NULL;
}
