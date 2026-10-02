#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue Queue_t;

/// Inizializza una coda con capacità massima specificata.
/// @param capacity Numero massimo di elementi che la coda può contenere.
/// @return Puntatore alla coda, oppure NULL se non è possibile allocarla.
Queue_t* initQueue(int capacity);

/// Inserisce un elemento nella coda.
/// @param q Puntatore alla coda.
/// @param item Puntatore all’elemento da inserire.
/// @return 0 se l’inserimento ha avuto successo, -1 se la coda è piena.
int push(Queue_t *q, void *item);

/// Rimuove e restituisce l’elemento in testa alla coda.
/// @param q Puntatore alla coda.
/// @return Puntatore all’elemento oppure NULL se la coda è vuota.
void* pop(Queue_t *q);

/// Restituisce l’elemento in testa alla coda senza rimuoverlo.
/// @param q Puntatore alla coda.
/// @return Puntatore all’elemento oppure NULL se la coda è vuota.
void* top(Queue_t *q);

/// Restituisce il numero di elementi attualmente presenti nella coda.
/// @param q Puntatore alla coda.
/// @return Numero di elementi presenti.
int length(Queue_t *q);

/// Libera la memoria occupata dalla coda.
/// @param q Puntatore alla coda.
void deleteQueue(Queue_t *q);

#endif
