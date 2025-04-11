#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "master.h"
#include "queue.h"

extern Queue_t *workQueue;
extern Queue_t *resultQueue;
extern pthread_mutex_t q_mutex;
extern pthread_cond_t not_empty;
extern pthread_cond_t not_full;
extern int finished;  /* 0 = false, 1 = true */

void *master_thread(void *arg) {
    Parameters *params = (Parameters*) arg;
    int current_index = 0;
    
    printf("[MASTER] Avvio del thread Master con N=%d, k=%d\n", params->N, params->k);
    
    while (current_index < params->N) {
        Group *group = malloc(sizeof(Group));
        if (!group) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        group->num_pairs = 0;
        printf("[MASTER] Creazione di un nuovo gruppo a partire dall'indice %d\n", current_index);
        
        while (group->num_pairs < params->k && (current_index + 1) < params->N) {
            group->pairs[group->num_pairs].a = params->array[current_index];
            group->pairs[group->num_pairs].b = params->array[current_index + 1];
            printf("[MASTER] Aggiunta coppia (%d, %d) al gruppo\n",
                   params->array[current_index],
                   params->array[current_index + 1]);
            group->num_pairs++;
            current_index += 2;
        }
        
        if ((current_index < params->N) && (params->N - current_index == 1)) {
            group->pairs[group->num_pairs].a = params->array[current_index];
            group->pairs[group->num_pairs].b = 0;
            printf("[MASTER] Aggiunta coppia incompleta (%d, 0) al gruppo\n",
                   params->array[current_index]);
            group->num_pairs++;
            current_index++;
        }
        
        pthread_mutex_lock(&q_mutex);
        while (length(workQueue) >= params->C) {
            printf("[MASTER] WorkQueue piena (%d elementi); attendo...\n", length(workQueue));
            pthread_cond_wait(&not_full, &q_mutex);
        }
        push(workQueue, group);
        printf("[MASTER] Inserito gruppo con %d coppie nella WorkQueue (size=%d)\n", group->num_pairs, length(workQueue));
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&q_mutex);
    }
    
    pthread_mutex_lock(&q_mutex);
    finished = 1;
    printf("[MASTER] Fine produzione. Notifico tutti i Worker.\n");
    pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&q_mutex);
    
    int total_sum = 0;
    for (int i = 0; i < params->numWorkers; i++) {
        pthread_mutex_lock(&q_mutex);
        while (length(resultQueue) == 0) {
            printf("[MASTER] Attendo risultati parziali, resultQueue vuota...\n");
            pthread_cond_wait(&not_empty, &q_mutex);
        }
        int *partial = (int *) pop(resultQueue);
        printf("[MASTER] Ricevuto risultato parziale: %d\n", *partial);
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&q_mutex);
        total_sum += *partial;
        free(partial);
    }
    
    printf("[MASTER] Somma finale: %d\n", total_sum);
    return NULL;
}
