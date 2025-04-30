// File: include/worker.h
#ifndef WORKER_H
#define WORKER_H

/**
 * @brief Funzione eseguita da ciascun thread Worker.
 *
 * Preleva gruppi di lavoro dalla coda, calcola la somma parziale,
 * e alla fine inserisce il risultato totale parziale nella coda prima di terminare.
 *
 * @param arg Puntatore alla struttura SharedQueue condivisa (castato da void*).
 * @return void* NULL in caso di successo (convenzione pthreads).
 */
void* worker_routine(void* arg);

/**
 * @brief Calcola la somma degli elementi in un WorkItem.
 * (Potrebbe stare in utils.h se preferito)
 * @param work Il WorkItem da processare.
 * @return long long La somma degli elementi.
 */
long long calculate_sum(WorkItem work);


#endif // WORKER_H