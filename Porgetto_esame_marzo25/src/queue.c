#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

void queue_init(queue_t *q, int capacity) {
    q->buffer = malloc(capacity * sizeof(int));
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    q->capacity = capacity;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->isFull, NULL);
    pthread_cond_init(&q->isEmpty, NULL);
}

void queue_destroy(queue_t *q) {
    free(q->buffer);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->isFull);
    pthread_cond_destroy(&q->isEmpty);
}

void queue_push(queue_t *q, int value) {
    pthread_mutex_lock(&q->mutex);

    while (q->size >= q->capacity) {
        pthread_cond_wait(&q->isFull, &q->mutex);
    }

    q->buffer[q->tail] = value;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;

    pthread_cond_signal(&q->isEmpty);
    pthread_mutex_unlock(&q->mutex);
}

int queue_pop(queue_t *q) {
    pthread_mutex_lock(&q->mutex);

    while (q->size == 0) {
        pthread_cond_wait(&q->isEmpty, &q->mutex);
    }

    int value = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;

    pthread_cond_signal(&q->isFull);
    pthread_mutex_unlock(&q->mutex);
    
    return value;
}

int queue_size(queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    int size = q->size;
    pthread_mutex_unlock(&q->mutex);
    return size;
}
