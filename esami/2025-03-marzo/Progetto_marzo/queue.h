#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>

/*
   Definizione della struttura Pair:
   Rappresenta una coppia di interi, usata per costruire un gruppo.
*/
typedef struct {
    int a;
    int b;
} Pair;

/*
   Definizione della struttura Group:
   Contiene un array dinamico di coppie (Pair) e tiene traccia di:
   - num_pairs: il numero corrente di coppie inserite.
   - max_pairs: il numero massimo di coppie (k) che il gruppo può contenere.
*/
typedef struct {
    Pair *pairs;
    int num_pairs;
    int max_pairs; // Valore di k
} Group;

/*
   Enumerazione ItemType:
   Specifica il tipo di oggetto che viene inserito nella coda.
   - ITEM_GROUP: indica che l'oggetto è un gruppo (Group *).
   - ITEM_RESULT: indica che l'oggetto è un risultato parziale (int *).
*/
typedef enum {
    ITEM_GROUP,
    ITEM_RESULT
} ItemType;

/*
   Definizione della struttura QueueItem:
   Ogni oggetto inserito nella coda è un QueueItem che contiene:
   - type: il tipo dell'oggetto (group o result).
   - data: una union che contiene o il puntatore al gruppo o al risultato.
*/
typedef struct {
    ItemType type;
    union {
        Group *group;
        int *result;
    } data;
} QueueItem;

/*
   Definizione della struttura Queue:
   Implementa una coda circolare che contiene puntatori a QueueItem.
   - head: indice per la rimozione (pop).
   - tail: indice per l'inserimento (push).
   - count: numero corrente di elementi nella coda.
   - capacity: dimensione massima della coda.
*/
typedef struct {
    QueueItem **items;    // Array di puntatori a QueueItem
    int head;
    int tail;
    int count;
    int capacity;
} Queue;

/*
   Dichiarazione delle funzioni per la gestione della coda:
   - queue_init: inizializza la coda con una data capacità.
   - queue_push: inserisce un oggetto nella coda.
   - queue_pop: rimuove e restituisce un oggetto dalla coda.
   - queue_peek: restituisce (senza rimuovere) il primo oggetto della coda.
   - queue_destroy: libera le risorse allocate per la coda.
*/
void queue_init(Queue *q, int capacity);
void queue_push(Queue *q, QueueItem *item);
QueueItem* queue_pop(Queue *q);
QueueItem* queue_peek(Queue *q);
void queue_destroy(Queue *q);

#endif
