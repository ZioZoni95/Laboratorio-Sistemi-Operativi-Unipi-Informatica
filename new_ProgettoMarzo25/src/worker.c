// src/worker.c
#include "worker.h"
#include "group.h"
#include <stdlib.h>

void* worker_run(void *arg) {
    WorkerData *data = (WorkerData*)arg;
    int partial = 0;

    while (1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        pthread_mutex_lock(&data->sync->mutex);

        while (queue_size(data->queue) == 0 && !data->sync->production_done) {
            printf("[%ld.%06ld] WORKER %d: Coda vuota, in attesa...\n",
                   tv.tv_sec, tv.tv_usec, data->id);
            pthread_cond_wait(&data->sync->not_empty, &data->sync->mutex);
        }

        if (data->sync->production_done && queue_size(data->queue) == 0) {
            int *result = malloc(sizeof(int));
            *result = partial;

            while (queue_push(data->queue, result) == -1) {
                printf("[%ld.%06ld] WORKER %d: Coda piena per risultato, attesa...\n",
                       tv.tv_sec, tv.tv_usec, data->id);
                pthread_cond_wait(&data->sync->not_full, &data->sync->mutex);
            }

            printf("[%ld.%06ld] WORKER %d: Terminato. Invio risultato: %d\n",
                   tv.tv_sec, tv.tv_usec, data->id, *result);

            pthread_cond_signal(&data->sync->all_done);
            pthread_mutex_unlock(&data->sync->mutex);
            break;
        }

        Group *group = queue_pop(data->queue);
        /*** Stampa delle coppie elaborate ***/
        printf("[%ld.%06ld] WORKER %d: Prelevato gruppo %d-%d. Coppie: [",
               tv.tv_sec, tv.tv_usec, data->id, group->start, group->end);

        // Formattazione delle coppie
        for(int i = group->start; i < group->end; i += 2) {
            if(i + 1 < group->end) {
                printf("(%d,%d)", group->array[i], group->array[i+1]);
                if(i + 2 < group->end) printf(", ");
            }
        }
        printf("]\n");
        /*************************************/


        pthread_cond_signal(&data->sync->not_full);
        pthread_mutex_unlock(&data->sync->mutex);

        partial += group_sum(group);
        printf("[%ld.%06ld] WORKER %d: Elaborato gruppo. Somma parziale: %d\n",
               tv.tv_sec, tv.tv_usec, data->id, partial);
    }

    return NULL;
}