#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct {
    int *buffer;
    int head, tail, size, capacity;
    pthread_mutex_t mutex;
    pthread_cond_t isFull;
    pthread_cond_t isEmpty;
} queue_t;

void queue_init(queue_t *q, int capacity);
void queue_destroy(queue_t *q);
void queue_push(queue_t *q, int value);
int queue_pop(queue_t *q);
int queue_size(queue_t *q);

#endif
