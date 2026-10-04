#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

// Nodo della lista singolarmente concatenata che implementa la coda.
typedef struct Node {
    Partition_Index_Task task;
    struct Node *next;
} Node;

// Coda concorrente Q della traccia (FIFO illimitata).
// push e pop sono thread-safe: un mutex protegge la lista e una variabile di
// condizione fa attendere i consumatori quando la coda è vuota.
struct ConcurrentQueue {
    Node *head;
    Node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond_non_empty;
    int closed;                    // 1 quando non verranno più inseriti task
};

// Inizializza la coda. Restituisce 0 se ha successo, -1 in caso di errore.
int init_queue(ConcurrentQueue *q);

// Libera i nodi rimasti e distrugge mutex e variabile di condizione.
// Nessun thread deve più usare la coda.
void destroy_queue(ConcurrentQueue *q);

// Inserisce un task in fondo alla coda.
void push(ConcurrentQueue *q, Partition_Index_Task task);

// Estrae il task in testa. Se la coda è vuota ma non chiusa attende.
// Restituisce 1 con il task in *task; 0 se la coda è vuota E chiusa.
int pop(ConcurrentQueue *q, Partition_Index_Task *task);

// Dichiara che non verranno più inseriti task e sveglia tutti i consumatori in attesa.
void close_queue(ConcurrentQueue *q);

#endif // QUEUE_H
