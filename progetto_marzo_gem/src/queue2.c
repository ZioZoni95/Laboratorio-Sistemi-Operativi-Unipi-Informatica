// File: src/queue2.c
#include "queue2.h"  // Prototipi e definizioni comuni
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Per memset (opzionale)
#include <errno.h>  // Per perror su errori pthread

// --- Funzioni Coda Concorrente ---

int queue_init(SharedQueue* q, int capacity, int num_workers) {
    if (q == NULL || capacity <= 0 || num_workers < 1) {
        fprintf(stderr, "queue_init: Argomenti non validi (q=%p, cap=%d, w=%d).\n", (void*)q, capacity, num_workers);
        return -1;
    }

    // Inizializza campi
    q->capacity = capacity;
    q->num_workers = num_workers;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->work_items_count = 0;
    q->results_in_queue_count = 0;
    q->master_done_producing = false;
    q->buffer = NULL; // Importante prima di malloc

    // Alloca buffer
    q->buffer = malloc(capacity * sizeof(QueueItem));
    if (q->buffer == NULL) {
        perror("queue_init: Errore malloc buffer");
        return -1;
    }

    // Inizializza mutex
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("queue_init: Errore mutex_init");
        free(q->buffer); q->buffer = NULL;
        return -1;
    }

    // Inizializza CV not_empty
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        perror("queue_init: Errore cond_init (not_empty)");
        pthread_mutex_destroy(&q->mutex); // Cleanup mutex
        free(q->buffer); q->buffer = NULL;
        return -1;
    }

    // Inizializza CV not_full
    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        perror("queue_init: Errore cond_init (not_full)");
        pthread_cond_destroy(&q->not_empty); // Cleanup CV precedente
        pthread_mutex_destroy(&q->mutex);    // Cleanup mutex
        free(q->buffer); q->buffer = NULL;
        return -1;
    }

    printf("Coda condivisa inizializzata (Capacità %d, Worker %d).\n", capacity, num_workers);
    return 0; // Successo
}

void queue_destroy(SharedQueue* q) {
    if (q == NULL) return;

    // Distruggi CV e Mutex (ignora errori per semplicità qui)
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    pthread_mutex_destroy(&q->mutex);

    // Libera buffer
    if (q->buffer != NULL) {
        free(q->buffer);
        q->buffer = NULL;
    }
    printf("Coda condivisa distrutta.\n");
}

// --- Funzione helper interna (ex push unsafe) ---
static void push_internal(SharedQueue* q, QueueItem item) {
    q->buffer[q->tail] = item;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    if(item.type == ITEM_TYPE_WORK) { q->work_items_count++; }
    else { q->results_in_queue_count++; }
}

// --- Funzione helper interna (ex pop unsafe) ---
static QueueItem pop_internal(SharedQueue* q) {
    QueueItem item = q->buffer[q->head];
    // Opzionale: memset(&q->buffer[q->head], 0, sizeof(QueueItem));
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    if (item.type == ITEM_TYPE_WORK) { q->work_items_count--; }
    else { q->results_in_queue_count--; }
    return item;
}

// Thread-safe enqueue
bool queue_enqueue(SharedQueue* q, QueueItem item) {
    if (q == NULL) return false;

    pthread_mutex_lock(&q->mutex);

    // Attendi finché la coda è piena E la produzione non è finita
    while (q->count >= q->capacity && !q->master_done_producing) {
        // printf("[%lx] Enqueue: Coda piena (%d/%d). Attendo not_full...\n", pthread_self(), q->count, q->capacity);
        pthread_cond_wait(&q->not_full, &q->mutex);
        // printf("[%lx] Enqueue: Risvegliato da not_full.\n", pthread_self());
    }

    // Controlla se è possibile inserire dopo l'attesa
    if (q->count >= q->capacity || (q->master_done_producing && item.type == ITEM_TYPE_WORK)) {
        // Impossibile inserire: o ancora piena o master ha finito e si tenta di aggiungere lavoro
        pthread_mutex_unlock(&q->mutex);
         if (q->master_done_producing && item.type == ITEM_TYPE_WORK)
            fprintf(stderr,"[Enqueue Warning] Master ha finito, impossibile aggiungere lavoro.\n");
         else
            fprintf(stderr,"[Enqueue Warning] Coda ancora piena dopo wait?\n");
        return false;
    }

    // Inserimento effettivo usando l'helper
    push_internal(q, item);
    // printf("[%lx] Enqueue: Item inserito (tipo %d). count=%d, work=%d, result=%d\n", pthread_self(), item.type, q->count, q->work_items_count, q->results_in_queue_count);


    // Segnala che non è vuota a UN thread in attesa
    pthread_cond_signal(&q->not_empty);

    pthread_mutex_unlock(&q->mutex);
    return true;
}

// Thread-safe dequeue
bool queue_dequeue(SharedQueue* q, QueueItem* out_item) {
     if (q == NULL || out_item == NULL) return false;

     pthread_mutex_lock(&q->mutex);

     // Attendi finché la coda è vuota E la produzione non è completa
     while (q->count <= 0 && !q->master_done_producing) {
        // printf("[%lx] Dequeue: Coda vuota (%d) e Master attivo. Attendo not_empty...\n", pthread_self(), q->count);
        pthread_cond_wait(&q->not_empty, &q->mutex);
        // printf("[%lx] Dequeue: Risvegliato da not_empty.\n", pthread_self());
     }

     // Se la coda è ancora vuota E la produzione è finita -> Fine lavoro
     if (q->count <= 0 /*&& q->master_done_producing - implicito*/) {
         pthread_mutex_unlock(&q->mutex);
         // printf("[%lx] Dequeue: Fine lavoro rilevata.\n", pthread_self());
         return false; // Segnale di fine lavoro per il chiamante
     }

     // Estrazione effettiva usando l'helper
     *out_item = pop_internal(q);
    //  printf("[%lx] Dequeue: Item estratto (tipo %d). count=%d, work=%d, result=%d\n", pthread_self(), out_item->type, q->count, q->work_items_count, q->results_in_queue_count);

     // Segnala che non è più piena a UN thread in attesa
     pthread_cond_signal(&q->not_full);

     pthread_mutex_unlock(&q->mutex);
     return true; // Elemento estratto con successo
}

// Segnala fine produzione
void queue_signal_master_done(SharedQueue* q) {
    if (q == NULL) return;
    pthread_mutex_lock(&q->mutex);
    if (!q->master_done_producing) { // Imposta solo una volta
        q->master_done_producing = true;
        // Sveglia TUTTI i thread potenzialmente in attesa
        pthread_cond_broadcast(&q->not_empty); // Sveglia i consumer/worker
        pthread_cond_broadcast(&q->not_full);  // Sveglia i producer (master o worker che inviano risultati)
        printf("\n[Queue] *** MASTER HA FINITO DI PRODURRE ***\n\n");
    }
    pthread_mutex_unlock(&q->mutex);
}