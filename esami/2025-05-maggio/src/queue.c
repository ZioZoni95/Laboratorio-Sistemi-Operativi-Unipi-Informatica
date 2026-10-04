/**
 * @file queue.c
 * @brief Coda concorrente (thread-safe) di task: lista concatenata + mutex + variabile di condizione.
 *
 * `push` e `pop` sono le operazioni della traccia. La coda può essere "chiusa":
 * dopo `close_queue` i consumatori che trovano la coda vuota terminano invece di
 * attendere un task che non arriverà mai.
 */

#include "queue.h"

int init_queue(ConcurrentQueue *q) {
    q->head = q->tail = NULL;
    q->closed = 0;
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("Errore inizializzazione mutex coda");
        return -1;
    }
    if (pthread_cond_init(&q->cond_non_empty, NULL) != 0) {
        perror("Errore inizializzazione cond var coda");
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    return 0;
}

void destroy_queue(ConcurrentQueue *q) {
    // Nessun altro thread deve usare la coda: non serve il lock.
    Node *current = q->head;
    while (current != NULL) {
        Node *next = current->next;
        free(current);
        current = next;
    }
    q->head = q->tail = NULL;
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_non_empty);
}

void push(ConcurrentQueue *q, Partition_Index_Task task) {
    Node *new_node = malloc(sizeof(Node));
    CHECK_ERR(new_node == NULL, "push: errore allocazione nodo");
    new_node->task = task;
    new_node->next = NULL;

    CHECK_PTHREAD_ERR(pthread_mutex_lock(&q->mutex), "push: lock");
    if (q->tail == NULL) {
        q->head = q->tail = new_node;
    } else {
        q->tail->next = new_node;
        q->tail = new_node;
    }
    // Un solo consumatore può usare questo task: ne basta uno sveglio.
    CHECK_PTHREAD_ERR(pthread_cond_signal(&q->cond_non_empty), "push: signal");
    CHECK_PTHREAD_ERR(pthread_mutex_unlock(&q->mutex), "push: unlock");
}

int pop(ConcurrentQueue *q, Partition_Index_Task *task) {
    CHECK_PTHREAD_ERR(pthread_mutex_lock(&q->mutex), "pop: lock");

    // `while` e non `if`: protegge dai risvegli spuri di pthread_cond_wait.
    while (q->head == NULL && !q->closed) {
        CHECK_PTHREAD_ERR(pthread_cond_wait(&q->cond_non_empty, &q->mutex), "pop: wait");
    }

    // Coda vuota qui significa che è stata chiusa.
    if (q->head == NULL) {
        CHECK_PTHREAD_ERR(pthread_mutex_unlock(&q->mutex), "pop: unlock");
        return 0;
    }

    Node *node = q->head;
    *task = node->task;
    q->head = node->next;
    if (q->head == NULL) q->tail = NULL;
    CHECK_PTHREAD_ERR(pthread_mutex_unlock(&q->mutex), "pop: unlock");

    free(node); // fuori dalla sezione critica
    return 1;
}

void close_queue(ConcurrentQueue *q) {
    CHECK_PTHREAD_ERR(pthread_mutex_lock(&q->mutex), "close_queue: lock");
    q->closed = 1;
    // Broadcast: TUTTI i consumatori in attesa devono rivalutare la condizione.
    CHECK_PTHREAD_ERR(pthread_cond_broadcast(&q->cond_non_empty), "close_queue: broadcast");
    CHECK_PTHREAD_ERR(pthread_mutex_unlock(&q->mutex), "close_queue: unlock");
}
