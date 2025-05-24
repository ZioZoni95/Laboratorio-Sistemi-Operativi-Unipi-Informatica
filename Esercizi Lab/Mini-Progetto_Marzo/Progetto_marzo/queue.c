#include "queue.h"

/*
   queue_init: Inizializza la coda impostando capacità, contatori e allocando l'array.
*/
void queue_init(Queue *q, int capacity) {
  q->capacity = capacity;
  q->count = 0;
  q->head = 0;
  q->tail = 0;
  q->items = malloc(capacity * sizeof(QueueItem *));
  if (!q->items) {
    perror("malloc queue items");
    exit(EXIT_FAILURE);
  }
}

/*
   queue_push: Inserisce un oggetto nella coda alla posizione 'tail' e aggiorna gli indici.
*/
void queue_push(Queue *q, QueueItem *item) {
  q->items[q->tail] = item;
  q->tail = (q->tail + 1) % q->capacity;
  q->count++;
}

/*
   queue_pop: Rimuove l'oggetto in testa alla coda e aggiorna gli indici.
   Restituisce il puntatore all'oggetto rimosso.
*/
QueueItem* queue_pop(Queue *q) {
  if(q->count == 0)
    return NULL;
  QueueItem *item = q->items[q->head];
  q->head = (q->head + 1) % q->capacity;
  q->count--;
  return item;
}

/*
   queue_peek: Restituisce il primo oggetto della coda senza rimuoverlo.
*/
QueueItem* queue_peek(Queue *q) {
  if(q->count == 0)
    return NULL;
  return q->items[q->head];
}

/*
   queue_destroy: Libera l'array allocato per la coda.
*/
void queue_destroy(Queue *q) {
  free(q->items);
}
