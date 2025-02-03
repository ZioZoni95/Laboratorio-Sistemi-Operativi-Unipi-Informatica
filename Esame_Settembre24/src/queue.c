#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

// Inizializza la coda con una capacità specifica
void queue_init(Queue_t *q, int capacity) {
    q->buffer = (long *)malloc(capacity * sizeof(long));
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = -1;
}

// Distrugge la coda liberando la memoria
void queue_destroy(Queue_t *q) {
    free(q->buffer);
}

// Controlla se la coda è vuota
bool queue_is_empty(Queue_t *q) {
    return q->size == 0;
}

// Controlla se la coda è piena
bool queue_is_full(Queue_t *q) {
    return q->size == q->capacity;
}

// Aggiunge un elemento in coda
void queue_put(Queue_t *q, long value) {
    if (queue_is_full(q)) {
        fprintf(stderr, "Errore: coda piena!\n");
        return;
    }
    q->rear = (q->rear + 1) % q->capacity;
    q->buffer[q->rear] = value;
    q->size++;
}

// Rimuove e restituisce un elemento dalla coda
long queue_get(Queue_t *q) {
    if (queue_is_empty(q)) {
        fprintf(stderr, "Errore: coda vuota!\n");
        return -1;  // Valore di errore
    }
    long value = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return value;
}