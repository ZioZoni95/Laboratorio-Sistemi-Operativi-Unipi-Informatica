#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>
#include "queue.h"

/*
 * Specifiche del progetto:
 *
 * Thread Master:
 *  - Suddivide l'array in gruppi di coppie (almeno 1, massimo k coppie per gruppo).
 *  - Inserisce i gruppi nella coda Q.
 *  - Notifica i thread Worker che non ci sono più gruppi e raccoglie i risultati parziali.
 *
 * Thread Worker:
 *  - Preleva dalla coda Q un gruppo di coppie e calcola la somma degli elementi del gruppo,
 *    aggiornando la propria somma parziale.
 *  - Quando non ci sono più gruppi da elaborare, invia il risultato parziale al Master e termina.
 *
 * La sincronizzazione viene gestita tramite mutex e variabili condizione.
 */

/* WorkerArgs: struttura per passare il worker_id a ciascun thread Worker. */
typedef struct {
    int worker_id;
} WorkerArgs;

/*
 * Variabili condivise (definite in threads.c):
 * - Q: la coda condivisa.
 * - mutex: per proteggere l'accesso a Q.
 * - cond_not_full: notifica quando Q non è piena (per operazioni push).
 * - cond_not_empty: notifica quando Q non è vuota (per operazioni pop).
 * - cond_results: utilizzata dal Master per attendere i risultati dai Worker.
 * - production_finished: flag che indica la fine della produzione dei gruppi da parte del Master.
 * - results_count: conta quanti risultati parziali sono stati inviati dai Worker.
 * - array, N, k, num_workers, capacity: parametri di configurazione (passati da riga di comando).
 */
extern Queue Q;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_not_full;
extern pthread_cond_t cond_not_empty;
extern pthread_cond_t cond_results;
extern int production_finished;
extern int results_count;
extern int *array;
extern int N, k, num_workers, capacity;

/* Variabile globale per la modalità di debug (0 = normale, 1 = debug). */
extern int debug_mode;

/* Dichiarazioni delle funzioni dei thread Master e Worker. */
void* thread_master(void *arg);
void* thread_worker(void *arg);

/*
 * Funzione di utility per stampare informazioni sullo stack del thread corrente.
 * Usa pthread_getattr_np (non standard, disponibile su Linux) per ottenere l'indirizzo base e la dimensione dello stack.
 */
void print_stack_info(const char *thread_name, int id);

#endif
