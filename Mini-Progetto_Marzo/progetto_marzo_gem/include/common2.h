// File: include/common.h
#ifndef COMMON2_H
#define COMMON2_H

#include <pthread.h> // Per tipi Pthreads (pthread_t, mutex_t, cond_t)
#include <stdbool.h> // Per il tipo bool
#include <stdio.h>   // Per NULL (e printf/perror usati altrove)
#include <stdlib.h>  // Per malloc, free, exit, atoi

// --- Strutture Dati Fondamentali ---

/**
 * @brief Rappresenta un'unità di lavoro per un worker.
 * Contiene un puntatore a una porzione dell'array globale V e il numero di elementi.
 */
typedef struct {
    long* data_ptr;     // Puntatore all'inizio dei dati per questo gruppo in V
    int num_elements;   // Numero di elementi long in questo gruppo (deve essere pari)
} WorkItem;

/**
 * @brief Rappresenta il risultato parziale calcolato da un worker.
 */
typedef struct {
    long long partial_sum; // La somma calcolata dal worker
} ResultItem;

/**
 * @brief Tipo di elemento contenuto nella coda condivisa.
 */
typedef enum {
    ITEM_TYPE_WORK,     // L'elemento contiene un WorkItem
    ITEM_TYPE_RESULT    // L'elemento contiene un ResultItem
} ItemType;

/**
 * @brief Struttura per un elemento nella coda condivisa.
 * Usa una union per contenere o un task di lavoro o un risultato.
 * Questo è necessario perché l'esercizio richiede una singola coda per entrambi.
 */
typedef struct {
    ItemType type;
    union {
        WorkItem work;
        ResultItem result;
    } data;
} QueueItem;

/**
 * @brief Struttura della coda concorrente condivisa.
 * Incapsula buffer, stato e meccanismi di sincronizzazione.
 */
typedef struct {
    QueueItem* buffer;          // Puntatore al buffer allocato dinamicamente
    int head;                   // Indice per dequeue (prossimo elemento da leggere)
    int tail;                   // Indice per enqueue (prossima posizione libera)
    int count;                  // Numero totale di elementi attualmente in coda
    int capacity;               // Capacità massima della coda (parametro C)

    pthread_mutex_t mutex;      // Mutex per proteggere l'accesso a TUTTI i campi di questa struct
    pthread_cond_t not_empty;   // CV per attendere se la coda è vuota
    pthread_cond_t not_full;    // CV per attendere se la coda è piena

    // Contatori specifici per la logica Master/Worker
    int work_items_count;       // Numero di WorkItem attualmente in coda (utile per il worker)
    int results_in_queue_count; // Numero di ResultItem attualmente in coda (utile per il master)

    // Stato della produzione
    bool master_done_producing; // Flag impostato dal Master quando ha finito di inviare WorkItem
    int num_workers;            // Numero totale di worker (parametro W)
} SharedQueue;

/**
 * @brief Struttura per passare gli argomenti ai thread worker.
 * Rende esplicite le dipendenze del worker.
 */
typedef struct {
    int worker_id;              // ID logico del worker (es. 0, 1, ...)
    SharedQueue* queue_ptr;     // Puntatore alla coda condivisa
    long* v_base_ptr;           // Puntatore all'inizio dell'array V globale
    int n_size;                 // Dimensione N dell'array V (per contesto/validazione)
} WorkerThreadArgs;


// --- Dichiarazioni Variabili Globali (definite in main.c) ---
// L'uso di 'extern' qui rende chiaro che queste variabili sono definite altrove
// e permette ad altri file .c che includono common.h di usarle.
extern long* V;  // L'array principale dei dati
extern int N;    // Dimensione dell'array V (usata da Master)
extern int k;    // Max coppie per gruppo (usato da Master)


// --- Prototipi Funzioni (definite nei rispettivi .c) ---

// queue.c - Funzioni della coda concorrente
int queue_init(SharedQueue* q, int capacity, int num_workers);
void queue_destroy(SharedQueue* q);
bool queue_enqueue(SharedQueue* q, QueueItem item);
bool queue_dequeue(SharedQueue* q, QueueItem* out_item);
void queue_signal_master_done(SharedQueue* q);

// master.c
void* master_routine(void* arg);
int min(int a, int b);

// worker.c
void* worker_routine(void* arg);
long long calculate_sum(WorkItem work);

#endif // COMMON2_H