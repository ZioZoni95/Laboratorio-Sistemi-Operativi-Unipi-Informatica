#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include <stdbool.h>
#include "queue.h"

//Costanti configurabili
extern int R ; //Capacità del porta ruote
extern int T ; // Capacità del porta telai
extern int B ; //Num biciclette da produrre

//Mutex e variabili di condizione
extern pthread_mutex_t muxruote, muxtelaio, muxmagazzino;
extern pthread_cond_t cvruote, cvtelai;
extern Queue_t qruote, qtelai;
extern long biciprodotte;

//Dichiarazioni delle funzioni
bool termina();
void deposita_ruota(long r);
void deposita_telaio(long t);
void preleva (long *ruota1, long *ruota2, long *telaio);
void deposita_magazzino();
void *wheelsproducer(void *arg);
void *chassisproducer (void *arg);
void *assemblerconsumer(void *arg);

#endif