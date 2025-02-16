//Queue.c - Implementazione della coda concorrente
#include <queue.h>
#include <stdlib.h>

//Inizializza la coda concorrente
void queue_init(Queue *q){
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

//Inserisce un valore della coda
void enqueue(Queue *q, int value){
    Node *new_node = malloc(sizeof(Node));
    new_node->value = value;
    new_node->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if(q->tail == NULL){
        q->tail = q->tail;
        q->tail = new_node;
    }else{
        q->tail->next = new_node;
        q->tail = new_node;
    }
    pthread_cond_signal(&q->cond); //Segnala ai consumatori che c'è un nuovo elemento
    pthread_mutex_unlock(&q->mutex);
}

//Estrae un valore dalla coda
int dequeue(Queue *q){
    pthread_mutex_lock(&q->mutex);
    while(q->head == NULL){ //Attende finchè la coda non ha elementi
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    Node *tmp = q->head;
    int value = tmp->value;
    q->head = q->head->next;
    if(q->head == NULL){
        q->tail = NULL;
    }
    free(tmp);

    pthread_mutex_unlock(&q->mutex);
    return value;
}

//Distrugge la coda e libera la memoria
void queue_destroy(Queue *q){
    while(q->head != NULL){
        Node *tmp = q-> head;
        q->head = q->head->next;
        free(tmp);
    }
    pthread_mutex_destroy(&q->mutex);
    pthead_cond_destroy(&q->cond);
}