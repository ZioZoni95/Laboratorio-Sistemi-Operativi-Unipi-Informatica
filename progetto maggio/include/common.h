#ifndef COMMON_H
#define COMMON_H

// --- Macro per abilitare/disabilitare le stampe di Debug ---
// Imposta a 1 per vedere stampe dettagliate (utile per debug)
// Imposta a 0 per un output pulito 
#define DEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

// --- Macro Utili ---

// Macro per controllo errori di sistema semplificato
#define CHECK_ERR(condition, message) \
    do { \
        if (condition) { \
            perror(message); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

// Macro per controllo errori funzioni pthread
#define CHECK_PTHREAD_ERR(err, message) \
    do { \
        if (err != 0) { \
            fprintf(stderr, "%s: %s\n", message, strerror(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

// Macro condizionali per le stampe di debug
#if DEBUG
    #define DEBUG_PRINT(tid, format, ...) printf("[Worker: %d] " format "\n", tid, ##__VA_ARGS__)
    #define DEBUG_PRINT_GEN(format, ...) printf("[PROG] " format "\n", ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(tid, format, ...) ((void)0) // Non fa nulla se DEBUG è 0
    #define DEBUG_PRINT_GEN(format, ...) ((void)0) // Non fa nulla se DEBUG è 0
#endif

// --- Strutture Dati Comuni ---

// Task: Rappresenta una partizione (intervallo di indici) da ordinare.
// Corrisponde alla "coppia di indici (start,end)
typedef struct {
    int start; // Indice iniziale della partizione
    int end;   // Indice finale della partizione
} Partition_Index_Task;

// Pre-dichiarazione della Coda Concorrente
typedef struct ConcurrentQueue ConcurrentQueue;

// ThreadArgs: Struttura per passare gli argomenti necessari a ciascun thread Worker.
typedef struct {
    int thread_id;          // ID univoco del thread (0 a P-1)
    int *array;             // Puntatore all'array condiviso da ordinare (N elementi)
    int *temp_array;        // Puntatore a un array temporaneo per la fase di merge
    long n_elements;        // Numero totale di elementi nell'array (N)
    int n_threads;          // Numero totale di thread Worker (P)
    ConcurrentQueue *queue; // Puntatore alla coda concorrente condivisa (Q)
    pthread_barrier_t *barrier; // Puntatore alla barriera di sincronizzazione condivisa
    pthread_mutex_t *merge_mutex_ptr; // Mutex per serializzare merge_sections su temp_array
    pthread_mutex_t *copy_phase_mutex_ptr; // Mutex per serializzare la fase di copia da temp_array ad array
} ThreadArgs;

#endif // COMMON_H