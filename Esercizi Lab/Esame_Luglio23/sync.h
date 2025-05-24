//sync.h - Dichiarazione delle funzioni di sincronizzazione
#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>
#include <semaphore.h>

//Dichiarazione dei semafori
extern sem_t mux, wait, waitT1T2, waitT3T4;

//Dichiarazione dei contatori
extern int T1T2, T1T2waiting, T3T4, T3T4waiting;

//Funzione di sincronizzazione
void accediT1T2();
void rilasciaT1T2();
void accediT3T4();
void rilasciaT3T4();

#endif //SYNC_C