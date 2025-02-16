//queue.h - Dichiarazioni della coda concorrente

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

//Struttura per un nodo della coda
typedef struct Node{
    int value; //Valore del nodo
    struct Node *next; //Puntatore al nodo successivo
}Node;

//Struttura per la coda concorrente
typedef struct{
    Node *front; //Puntatore alla testa della coda
    Node *rear; //Puntatore all'ultimo nodo della coda
    pthread_mutex_t mutex; //Mutex per la sincronizzazione
    pthread_cond_t cond; //Variabile di condizione per la sincronizzazione
}Queue;

//Funzioni per la gestione della coda
void queue_init(Queue *q);
void enqueue(Queue *q, int value);
int dequeue(Queue *q);
void queue_destroy(Queue *q);

#endif //QUEUE_Q