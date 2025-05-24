#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "worker.h"
#include "queue.h"

extern Queue_t *workQueue;
extern Queue_t *resultQueue;
extern pthread_mutex_t q_mutex;
extern pthread_cond_t not_empty;
extern pthread_cond_t not_full;
extern int finished;  /* 0 = false, 1 = true */

void *worker_thread(void *arg) {
    WorkerArg *warg = (WorkerArg *) arg;
    int worker_id = warg->worker_id;
    Parameters *params = warg->params;
    int local_sum = 0;
    
    printf("[WORKER %d] Avvio del thread Worker\n", worker_id);
    
    while (1) {
        pthread_mutex_lock(&q_mutex);
        while (length(workQueue) == 0 && !finished) {
            printf("[WORKER %d] WorkQueue vuota, attendo...\n", worker_id);
            pthread_cond_wait(&not_empty, &q_mutex);
        }
        if (length(workQueue) == 0 && finished) {
            pthread_mutex_unlock(&q_mutex);
            printf("[WORKER %d] Nessun lavoro e produzione finita, termino.\n", worker_id);
            break;
        }
        Group *group = (Group *) pop(workQueue);
        printf("[WORKER %d] Estratto un gruppo dalla WorkQueue (rimanenti=%d)\n", worker_id, length(workQueue));
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&q_mutex);
        
        for (int i = 0; i < group->num_pairs; i++) {
            int a = group->pairs[i].a;
            int b = group->pairs[i].b;
            local_sum += a + b;
            printf("[WORKER %d] Elaboro coppia (%d, %d) -> somma parziale = %d\n", worker_id, a, b, local_sum);
        }
        free(group);
    }
    
    int *result = malloc(sizeof(int));
    if (!result) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    *result = local_sum;
    printf("[WORKER %d] Totale parziale = %d, inserisco risultato nella ResultQueue\n", worker_id, local_sum);
    
    pthread_mutex_lock(&q_mutex);
    while (length(resultQueue) >= params->C) {
        printf("[WORKER %d] ResultQueue piena; attendo...\n", worker_id);
        pthread_cond_wait(&not_full, &q_mutex);
    }
    push(resultQueue, result);
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&q_mutex);
    
    printf("[WORKER %d] Fine esecuzione del thread Worker\n", worker_id);
    return NULL;
}
