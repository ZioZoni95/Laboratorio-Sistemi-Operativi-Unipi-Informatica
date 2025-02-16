//Queue.c - Implementazione della coda concorrente
#include <queue.h>
#include <stdlib.h>

// Inizializza la coda concorrente
void queue_init(Queue* q) {
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

// Inserisce un valore nella coda
void enqueue(Queue* q, int value) {
    Node* new_node = malloc(sizeof(Node));
    new_node->value = value;
    new_node->next = NULL;
    
    pthread_mutex_lock(&q->mutex);
    if (q->rear == NULL) {
        q->front = q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
    pthread_cond_signal(&q->cond); // Segnala ai consumatori che c'è un nuovo elemento
    pthread_mutex_unlock(&q->mutex);
}

// Estrae un valore dalla coda
int dequeue(Queue* q) {
    pthread_mutex_lock(&q->mutex);
    while (q->front == NULL) { // Attende finché la coda non ha elementi
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    
    Node* temp = q->front;
    int value = temp->value;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    
    pthread_mutex_unlock(&q->mutex);
    return value;
}

// Distrugge la coda e libera la memoria
void queue_destroy(Queue* q) {
    while (q->front != NULL) {
        Node* temp = q->front;
        q->front = q->front->next;
        free(temp);
    }
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}