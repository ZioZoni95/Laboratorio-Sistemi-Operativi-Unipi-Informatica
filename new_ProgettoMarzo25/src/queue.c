// src/queue.c
#include "queue.h"
#include <stdlib.h>
#include <assert.h>

Queue* queue_init(int capacity) {
    Queue *q = malloc(sizeof(Queue));
    if (!q) return NULL;

    q->head = malloc(sizeof(Node));
    if (!q->head) { free(q); return NULL; }
    q->head->data = NULL;
    q->head->next = NULL;
    q->tail = q->head;
    q->size = 0;
    q->capacity = capacity;
    return q;
}

void queue_destroy(Queue *q) {
    Node *current = q->head;
    while (current) {
        Node *temp = current;
        current = current->next;
        free(temp);
    }
    free(q);
}

int queue_push(Queue *q, void *data) {
    if (q->size >= q->capacity) return -1; // Coda piena

    Node *new_node = malloc(sizeof(Node));
    if (!new_node) return -1;
    new_node->data = data;
    new_node->next = NULL;

    q->tail->next = new_node;
    q->tail = new_node;
    q->size++;
    return 0;
}

void* queue_pop(Queue *q) {
    if (q->size == 0) return NULL; // Coda vuota

    Node *old_head = q->head;
    void *data = old_head->next->data;
    q->head = old_head->next;
    q->size--;
    free(old_head);
    return data;
}

unsigned long queue_size(const Queue *q) {
    return q->size;
}