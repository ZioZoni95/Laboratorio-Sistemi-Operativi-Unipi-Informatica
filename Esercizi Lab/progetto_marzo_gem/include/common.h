// File: include/common.h
#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Per getopt

// --- Strutture Dati ---
// (Come definite nel Passo 2)

typedef struct {
    long* data_ptr;
    int num_elements;
} WorkItem;

typedef struct {
    long long partial_sum;
} ResultItem;

typedef enum {
    ITEM_TYPE_WORK,
    ITEM_TYPE_RESULT
} ItemType;

typedef struct {
    ItemType type;
    union {
        WorkItem work;
        ResultItem result;
    } data;
} QueueItem;

typedef struct {
    QueueItem* buffer; // Puntatore al buffer allocato dinamicamente
    int head;
    int tail;
    int count;
    int capacity; // C
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int work_items_count;
    int results_in_queue_count;
    bool master_done_producing;
    int num_workers; // W
} SharedQueue;

// --- Prototipi Funzioni (Esempio) ---
// Metteremo qui i prototipi delle funzioni definite in altri file .c
// quando li creeremo. Ad esempio:
// void queue_init(SharedQueue* q, int capacity, int num_workers);
// void queue_destroy(SharedQueue* q);
// void* worker_routine(void* arg);
// void* master_routine(void* arg); // O la logica del master è nel main

#endif // COMMON_H