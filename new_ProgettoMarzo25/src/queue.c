#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "queue.h"  // Si assume che il file queue.h definisca l’interfaccia per Queue_t

// Definizione della struttura della coda (implementazione con buffer circolare)
struct queue {
    void **items;       // Array di puntatori agli elementi
    int capacity;       // Capacità massima della coda
    int front;          // Indice del primo elemento
    int rear;           // Indice per l'inserimento del prossimo elemento
    int count;          // Numero di elementi correnti nella coda
};

/// @brief Inizializza una coda non concorrente con la capacità specificata.
/// @param capacity Dimensione massima della coda.
/// @return Puntatore alla nuova coda allocata, oppure NULL in caso di errore.
Queue_t* initQueue(int capacity) {
    Queue_t *q = malloc(sizeof(Queue_t));
    if (!q) {
        perror("malloc");
        return NULL;
    }
    q->items = malloc(sizeof(void*) * capacity);
    if (!q->items) {
        free(q);
        perror("malloc");
        return NULL;
    }
    q->capacity = capacity;
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    return q;
}

/// @brief Inserisce un elemento nella coda.
/// @param q Puntatore alla coda.
/// @param item Puntatore all'elemento da inserire.
/// @return 0 se l'inserimento ha avuto successo, -1 se la coda è piena.
int push(Queue_t *q, void *item) {
    if(q->count == q->capacity) {
        // Coda piena
        return -1;
    }
    q->items[q->rear] = item;
    q->rear = (q->rear + 1) % q->capacity;
    q->count++;
    return 0;
}

/// @brief Estrae (rimuove) e restituisce l’elemento in testa alla coda.
/// @param q Puntatore alla coda.
/// @return Puntatore all’elemento in testa, oppure NULL se la coda è vuota.
void* pop(Queue_t *q) {
    if(q->count == 0) {
        // Coda vuota
        return NULL;
    }
    void* item = q->items[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->count--;
    return item;
}

/// @brief Restituisce (senza rimuovere) l’elemento in testa alla coda.
/// @param q Puntatore alla coda.
/// @return Puntatore all’elemento in testa, oppure NULL se la coda è vuota.
void* top(Queue_t *q) {
    if(q->count == 0) {
        return NULL;
    }
    return q->items[q->front];
}

/// @brief Restituisce il numero di elementi attualmente presenti nella coda.
/// @param q Puntatore alla coda.
/// @return Numero di elementi in coda.
int length(Queue_t *q) {
    return q->count;
}

/// @brief Libera la memoria allocata per la coda.
/// @param q Puntatore alla coda.
void deleteQueue(Queue_t *q) {
    if(q) {
        free(q->items);
        free(q);
    }
}
