// File: include/master.h
#ifndef MASTER_H
#define MASTER_H

/**
 * @brief Funzione eseguita dal thread Master (o dal main thread che agisce come Master).
 *
 * Gestisce la produzione dei gruppi di lavoro, l'inserimento nella coda,
 * la notifica di fine produzione e la raccolta dei risultati dai worker.
 *
 * @param arg Puntatore alla struttura SharedQueue condivisa (castato da void*).
 * @return void* NULL in caso di successo (convenzione pthreads).
 */
void* master_routine(void* arg);

// Funzione helper per min (potrebbe stare in utils.h)
int min(int a, int b);

#endif // MASTER_H