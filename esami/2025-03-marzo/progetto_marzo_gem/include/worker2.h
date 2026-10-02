// File: include/worker.h
#ifndef WORKER2_H
#define WORKER2_H

#include "common2.h" // Per WorkItem, WorkerThreadArgs, ecc.

/**
 * @brief Funzione eseguita da ciascun thread Worker.
 * Preleva task dalla coda, calcola la somma parziale degli elementi,
 * e alla fine inserisce il risultato totale parziale nella coda prima di terminare.
 * @param arg Puntatore a WorkerThreadArgs (castato da void*).
 * @return void* NULL in caso di successo.
 */
void* worker_routine(void* arg);

/**
 * @brief Calcola la somma degli elementi specificati da un WorkItem.
 * @param work Il WorkItem contenente il puntatore ai dati e il numero di elementi.
 * @return long long La somma degli elementi.
 */
long long calculate_sum(WorkItem work);

#endif // WORKER2_H