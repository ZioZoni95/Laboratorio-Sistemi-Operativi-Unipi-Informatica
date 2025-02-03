#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include <stdbool.h>
#include "queue.h"

//Costanti configurabili
#define R 5 //Capacità del porta ruote
#define T 3 // Capacità del porta telai
#define B 10 //Num biciclette da produrre

//Mutex e variabili di condizione
extern pthread_mutex_t muxruote, muxtelaio, muxdeposito;
extern pthread_cond_t cvruote, cvtelai;
extern Queue_t qruote, qtelai;
extern long biciprodotte;

//Dichiarazioni delle funzioni
bool termina();
void deposita_ruota(long r);
void deposita_telaio(long t);
void preleva (long *ruota1, long *ruota2, long *telaio);
void deposita_magazzino();
void *francesco(void *arg);
void *federico (void *arg);
void *franco(void *arg);

#endif