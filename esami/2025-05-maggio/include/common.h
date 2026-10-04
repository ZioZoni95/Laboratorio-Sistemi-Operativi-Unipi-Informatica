#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// --- Verbosità ---
// Livello scelto a runtime con l'opzione -v (0 = silenzioso, default).
// Con -v i Worker stampano le partizioni ordinate e i blocchi fusi a ogni passo.
extern int g_verbose;

#define LOG(tid, format, ...) \
    do { \
        if (g_verbose) printf("[Worker %d] " format "\n", (tid), ##__VA_ARGS__); \
    } while (0)

// --- Controllo errori ---

// Errore di una chiamata di sistema/libreria che imposta errno.
#define CHECK_ERR(condition, message) \
    do { \
        if (condition) { \
            perror(message); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

// Errore di una funzione pthread (restituisce direttamente il codice d'errore).
#define CHECK_PTHREAD_ERR(err, message) \
    do { \
        int err_ = (err); \
        if (err_ != 0) { \
            fprintf(stderr, "%s: %s\n", (message), strerror(err_)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

// --- Strutture dati ---

// Task: una partizione dell'array da ordinare, come coppia di indici (start,end)
// con estremi INCLUSI, come richiesto dalla traccia.
typedef struct {
    long start;
    long end;
} Partition_Index_Task;

// Dichiarazione anticipata della coda concorrente (definita in queue.h).
typedef struct ConcurrentQueue ConcurrentQueue;

// Argomenti passati a ciascun Worker.
typedef struct {
    int thread_id;              // identificativo del Worker (0 .. n_threads-1)
    int n_threads;              // numero di Worker (P)
    long n_partitions;          // numero di partizioni iniziali (Q, di default Q = P)
    long n_elements;            // dimensione dell'array (N)
    int *array;                 // array da ordinare (N elementi)
    int *temp_array;            // buffer di appoggio per i merge (N elementi)
    ConcurrentQueue *queue;     // coda concorrente dei task
    pthread_barrier_t *barrier; // barriera condivisa fra i P Worker
} ThreadArgs;

#endif // COMMON_H
