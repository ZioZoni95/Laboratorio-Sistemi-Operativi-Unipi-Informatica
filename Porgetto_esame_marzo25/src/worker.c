#include <stdio.h>
#include <pthread.h>
#include "queue.h"
#include "util.h"

extern pthread_mutex_t mutex;
extern pthread_cond_t isFull;
extern pthread_cond_t isEmpty;
extern queue_t Q;
extern int produzione_terminata;
extern int k;

void *T_Worker(void *arg) {
    pthread_t tid = pthread_self();

    while (1) {
        pthread_mutex_lock(&mutex);

        while (queue_size(&Q) < 2 * k && !produzione_terminata) {
            printf("[Worker-%lu] 🟡 Coda vuota o gruppo incompleto, attendo...\n", tid);
            pthread_cond_wait(&isEmpty, &mutex);
        }

        if (produzione_terminata && queue_size(&Q) < 2 * k) {
            pthread_mutex_unlock(&mutex);
            printf("[Worker-%lu] ⛔ Terminazione rilevata, esco.\n", tid);
            break;
        }

        int gruppo[2 * k];
        for (int j = 0; j < 2 * k; j++) {
            gruppo[j] = queue_pop(&Q);
        }

        printf("[Worker-%lu] ✅ Prelevato gruppo di %d elementi dalla coda\n", tid, 2 * k);
        pthread_cond_signal(&isFull);
        pthread_mutex_unlock(&mutex);

        int somma = calcola_somma_parziale(gruppo, 2 * k);
        printf("[Worker-%lu] 🟢 Somma parziale calcolata: %d\n", tid, somma);

        pthread_mutex_lock(&mutex);
        queue_push(&Q, somma);
        pthread_cond_signal(&isEmpty);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}
