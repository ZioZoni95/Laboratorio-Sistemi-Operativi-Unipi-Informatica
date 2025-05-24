// File: include/queue.h
#ifndef QUEUE2_H
#define QUEUE2_H

#include "common2.h" // Include SharedQueue, QueueItem, bool, ecc.

// --- Prototipi Funzioni Coda Concorrente (Thread-Safe) ---

/**
 * @brief Inizializza una struttura SharedQueue concorrente.
 * Alloca buffer, inizializza mutex, CV, contatori, ecc.
 * @param q Puntatore alla SharedQueue.
 * @param capacity Capacità massima C.
 * @param num_workers Numero worker W.
 * @return 0 successo, -1 errore.
 */
int queue_init(SharedQueue* q, int capacity, int num_workers);

/**
 * @brief Distrugge una SharedQueue concorrente.
 * Libera buffer, distrugge mutex e CV. La coda non deve essere in uso.
 * @param q Puntatore alla SharedQueue.
 */
void queue_destroy(SharedQueue* q);

/**
 * @brief Inserisce un elemento nella coda in modo thread-safe.
 * Si blocca (attende su CV not_full) se la coda è piena.
 * @param q Puntatore alla SharedQueue.
 * @param item L'elemento QueueItem da inserire.
 * @return true se successo, false se la produzione è terminata (`master_done_producing` è true e si tenta di aggiungere WORK)
 * o se si verifica un errore interno.
 */
bool queue_enqueue(SharedQueue* q, QueueItem item);

/**
 * @brief Estrae un elemento dalla coda in modo thread-safe.
 * Si blocca (attende su CV not_empty) se la coda è vuota.
 * @param q Puntatore alla SharedQueue.
 * @param out_item Puntatore dove memorizzare l'elemento estratto.
 * @return true se un elemento è stato estratto, false se la produzione è terminata
 * (`master_done_producing` è true) E la coda è vuota (segnale di fine lavoro per i worker).
 */
bool queue_dequeue(SharedQueue* q, QueueItem* out_item);

/**
 * @brief Segnala che il Master ha finito di produrre WorkItem.
 * Imposta il flag interno `master_done_producing` e sveglia (broadcast)
 * tutti i thread in attesa sulla coda (sia `not_empty` che `not_full`).
 * @param q Puntatore alla SharedQueue.
 */
void queue_signal_master_done(SharedQueue* q);

#endif // QUEUE2_H