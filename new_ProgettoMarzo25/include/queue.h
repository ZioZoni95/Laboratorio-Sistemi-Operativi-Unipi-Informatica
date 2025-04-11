#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <errno.h>

/**
 * @brief Nodo della coda.
 *
 * Il nodo contiene un puntatore generico ai dati e un puntatore al nodo successivo.
 */
typedef struct Node {
    void *data;
    struct Node *next;
} Node_t;

/**
 * @brief Struttura dati per la coda NON concorrente.
 *
 * La coda utilizza un nodo dummy per semplificare inserimenti e rimozioni.
 */
typedef struct Queue {
    Node_t *head;
    Node_t *tail;
    unsigned long q_len; // Numero di elementi effettivi (escluso il nodo dummy)
} Queue_t;

/**
 * @brief Inizializza una nuova coda NON concorrente.
 *
 * Alloca la struttura della coda ed il nodo dummy.
 *
 * @return Puntatore alla coda oppure NULL in caso di errore.
 */
Queue_t *initQueue();

/**
 * @brief Libera tutte le risorse allocate per la coda.
 *
 * Rimuove tutti i nodi (incluso quello dummy) e libera la memoria.
 *
 * @param q Puntatore alla coda da eliminare.
 */
void deleteQueue(Queue_t *q);

/**
 * @brief Inserisce un nuovo elemento nella coda.
 *
 * @param q Puntatore alla coda.
 * @param data Puntatore ai dati da inserire.
 * @return 0 in caso di successo, -1 in caso di errore.
 */
int push(Queue_t *q, void *data);

/**
 * @brief Rimuove e restituisce il primo elemento disponibile.
 *
 * Se la coda è vuota (cioè contiene solo il nodo dummy), restituisce NULL.
 *
 * @param q Puntatore alla coda.
 * @return Puntatore ai dati o NULL in caso di errore o coda vuota.
 */
void *pop(Queue_t *q);

/**
 * @brief Restituisce il numero di elementi presenti nella coda (escludendo il dummy).
 *
 * @param q Puntatore alla coda.
 * @return Numero di elementi presenti.
 */
unsigned long length(Queue_t *q);

#endif  // QUEUE_H
