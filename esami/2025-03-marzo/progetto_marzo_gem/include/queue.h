// File: include/queue.h
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h" // Include le definizioni delle struct e include comuni

// --- Prototipi Funzioni Coda ---

/**
 * @brief Inizializza una struttura SharedQueue.
 *
 * Alloca il buffer della coda, inizializza gli indici, i contatori,
 * il mutex e le variabili di condizione.
 *
 * @param q Puntatore alla struttura SharedQueue da inizializzare.
 * @param capacity La capacità massima della coda (C).
 * @param num_workers Il numero totale di thread worker (W).
 * @return 0 in caso di successo, -1 in caso di errore (es. fallimento malloc o init).
 */
int queue_init(SharedQueue* q, int capacity, int num_workers);

/**
 * @brief Distrugge una struttura SharedQueue.
 *
 * Distrugge il mutex e le variabili di condizione, e libera la memoria
 * allocata per il buffer della coda.
 * La coda NON deve essere in uso da nessun thread quando viene chiamata.
 *
 * @param q Puntatore alla struttura SharedQueue da distruggere.
 */
void queue_destroy(SharedQueue* q);

/**
 * @brief Inserisce un elemento nella coda condivisa.
 *
 * !!! NOTA: Questa funzione DEVE essere chiamata all'interno di una sezione critica
 * protetta dal mutex della coda (q->mutex).
 * Aggiorna gli indici, i contatori totali e specifici per tipo.
 * Non gestisce l'attesa se la coda è piena (gestita dal chiamante con CV).
 *
 * @param q Puntatore alla SharedQueue.
 * @param item L'elemento QueueItem da inserire.
 */
void push(SharedQueue* q, QueueItem item);

/**
 * @brief Estrae un elemento dalla coda condivisa.
 *
 * !!! NOTA: Questa funzione DEVE essere chiamata all'interno di una sezione critica
 * protetta dal mutex della coda (q->mutex).
 * Aggiorna gli indici, i contatori totali e specifici per tipo.
 * Non gestisce l'attesa se la coda è vuota (gestita dal chiamante con CV).
 *
 * @param q Puntatore alla SharedQueue.
 * @return L'elemento QueueItem estratto.
 */
QueueItem pop(SharedQueue* q);


#endif // QUEUE_H