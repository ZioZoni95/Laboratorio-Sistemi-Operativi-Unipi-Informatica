// File: include/master.h
#ifndef MASTER2_H
#define MASTER2_H

#include "common2.h" // Per SharedQueue, ecc.

/**
 * @brief Funzione eseguita dal thread Master (o dal main thread).
 * Gestisce produzione task, segnalazione fine, raccolta risultati.
 * @param arg Puntatore alla SharedQueue condivisa (castato da void*).
 * Potrebbe essere modificato per accettare una struct MasterArgs se necessario.
 * @return void* NULL in caso di successo.
 */
void* master_routine(void* arg);

/**
 * @brief Funzione helper per trovare il minimo tra due interi.
 * @param a Primo intero.
 * @param b Secondo intero.
 * @return Il minore tra a e b.
 */
int min(int a, int b);

#endif // MASTER2_H