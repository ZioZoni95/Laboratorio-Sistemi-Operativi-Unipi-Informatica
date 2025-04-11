/**
 * @file master.c
 * @brief Implementazione del thread Master con logging dettagliato
 */

#include "master.h"
#include "group.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void* master_run(void *arg) {
    MasterData *data = (MasterData*)arg;
    struct timeval tv;

    // 1. Creazione array dinamico (1..N)
    int *array = malloc(data->N * sizeof(int));
    if(!array) {
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] MASTER: Errore allocazione array\n", tv.tv_sec, tv.tv_usec);
        return NULL;
    }

    for(int i = 0; i < data->N; i++)
        array[i] = i + 1; // Popola con valori 1..N

    // 2. Suddivisione in gruppi
    int total_groups;
    Group *groups = group_split(array, data->N, data->k, &total_groups);
    if(!groups) {
        free(array);
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] MASTER: Errore creazione gruppi\n", tv.tv_sec, tv.tv_usec);
        return NULL;
    }

    // 3. Log iniziale
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] MASTER: Inizio elaborazione\n", tv.tv_sec, tv.tv_usec);
    printf("[%ld.%06ld] MASTER: Array creato [", tv.tv_sec, tv.tv_usec);
    for(int i = 0; i < data->N; i++) {
        printf("%d", array[i]);
        if(i < data->N - 1) printf(", ");
    }
    printf("]\n");
    printf("[%ld.%06ld] MASTER: Generati %d gruppi (k=%d coppie/gruppo)\n",
           tv.tv_sec, tv.tv_usec, total_groups, data->k);

    // 4. Stampa dettagli gruppi
    for(int i = 0; i < total_groups; i++) {
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] MASTER: Gruppo %d - Coppie: [", tv.tv_sec, tv.tv_usec, i);

        for(int j = groups[i].start; j < groups[i].end; j += 2) {
            if(j + 1 < groups[i].end)
                printf("(%d,%d)", array[j], array[j+1]);
            if(j + 2 < groups[i].end) printf(", ");
        }
        printf("] (indici %d-%d)\n", groups[i].start, groups[i].end);
    }

    // 5. Inserimento gruppi nella coda
    for(int i = 0; i < total_groups; i++) {
        pthread_mutex_lock(&data->sync->mutex);

        // Attende se la coda è piena
        while(queue_size(data->queue) >= data->queue->capacity) {
            gettimeofday(&tv, NULL);
            printf("[%ld.%06ld] MASTER: Coda piena (%lu/%d), in attesa...\n",
                   tv.tv_sec, tv.tv_usec, queue_size(data->queue), data->queue->capacity);
            pthread_cond_wait(&data->sync->not_full, &data->sync->mutex);
        }

        // Inserisce il gruppo
        queue_push(data->queue, &groups[i]);
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] MASTER: Inserito gruppo %d in coda\n",
               tv.tv_sec, tv.tv_usec, i);

        pthread_cond_signal(&data->sync->not_empty);
        pthread_mutex_unlock(&data->sync->mutex);
    }

    // 6. Notifica fine produzione
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] MASTER: Fine produzione gruppi\n", tv.tv_sec, tv.tv_usec);
    pthread_mutex_lock(&data->sync->mutex);
    data->sync->production_done = true;
    pthread_cond_broadcast(&data->sync->not_empty);
    pthread_mutex_unlock(&data->sync->mutex);

    // 7. Raccolta risultati
    int total = 0;
    for(int i = 0; i < data->worker_count; i++) {
        pthread_mutex_lock(&data->sync->mutex);

        while(queue_size(data->queue) == 0) {
            gettimeofday(&tv, NULL);
            printf("[%ld.%06ld] MASTER: Attesa risultati parziali...\n",
                   tv.tv_sec, tv.tv_usec);
            pthread_cond_wait(&data->sync->all_done, &data->sync->mutex);
        }

        int *partial = queue_pop(data->queue);
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] MASTER: Ricevuto risultato parziale: %d\n",
               tv.tv_sec, tv.tv_usec, *partial);

        total += *partial;
        free(partial);
        pthread_mutex_unlock(&data->sync->mutex);
    }

    // 8. Output finale e cleanup
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] MASTER: Somma finale calcolata: %d\n",
           tv.tv_sec, tv.tv_usec, total);

    free(groups);
    free(array);
    return NULL;
}