#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

// Struttura per la coda
typedef struct {
    long *buffer;      // Array di elementi
    int capacity;      // Dimensione massima
    int size;          // Elementi attuali nella coda
    int front;         // Indice del primo elemento
    int rear;          // Indice dell'ultimo elemento
} Queue_t;

// Funzioni della coda
void queue_init(Queue_t *q, int capacity);
void queue_destroy(Queue_t *q);
bool queue_is_empty(Queue_t *q);
bool queue_is_full(Queue_t *q);
void queue_put(Queue_t *q, long value);
long queue_get(Queue_t *q);

#endif