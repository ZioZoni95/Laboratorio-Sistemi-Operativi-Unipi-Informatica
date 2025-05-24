#ifndef THREADS_TYPES_H
#define THREADS_TYPES_H

#define MAX_K 32

/* Struttura che rappresenta una coppia */
typedef struct {
    int a;
    int b;
} Pair;

/* Gruppo di coppie */
typedef struct {
    Pair pairs[MAX_K];
    int num_pairs;
} Group;

/* Parametri del programma */
typedef struct {
    int N;          /* Lunghezza dell'array */
    int k;          /* Numero massimo di coppie per gruppo */
    int C;          /* Capacità massima delle code */
    int numWorkers; /* Numero di thread Worker */
    int *array;     /* Array degli interi da sommare */
} Parameters;

/* Struttura usata per passare i parametri e l'id al Worker */
typedef struct {
    Parameters *params;
    int worker_id;
} WorkerArg;

#endif
