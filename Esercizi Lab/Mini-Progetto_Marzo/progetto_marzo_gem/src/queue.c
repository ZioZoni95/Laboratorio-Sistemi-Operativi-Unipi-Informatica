// File: src/queue.c
#include "queue.h"  // Includi l'header corrispondente
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>  // Necessario per controllare errori pthread_*

// Implementazione di queue_init
int queue_init(SharedQueue* q, int capacity, int num_workers) {
    // Controllo preliminare degli argomenti
    if (q == NULL || capacity <= 0 || num_workers < 1) {
        fprintf(stderr, "queue_init: Argomenti non validi.\n");
        return -1; // Codice di errore
    }

    // Inizializza i campi della struttura
    q->capacity = capacity;
    q->num_workers = num_workers;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->work_items_count = 0;
    q->results_in_queue_count = 0;
    q->master_done_producing = false;
    q->buffer = NULL; // Importante inizializzarlo a NULL prima di malloc

    // Alloca dinamicamente il buffer della coda
    q->buffer = malloc(capacity * sizeof(QueueItem));
    if (q->buffer == NULL) {
        perror("queue_init: Errore malloc buffer coda");
        return -1; // Errore di allocazione memoria
    }

    // Inizializza il mutex
    // Il secondo argomento NULL usa gli attributi di default del mutex
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("queue_init: Errore pthread_mutex_init");
        free(q->buffer); // Libera la memoria già allocata prima di uscire
        q->buffer = NULL;
        return -1; // Errore inizializzazione mutex
    }

    // Inizializza la variabile di condizione "not_empty"
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        perror("queue_init: Errore pthread_cond_init (not_empty)");
        // Se fallisce l'init della CV, dobbiamo distruggere il mutex già inizializzato
        pthread_mutex_destroy(&q->mutex);
        free(q->buffer);
        q->buffer = NULL;
        return -1; // Errore inizializzazione CV
    }

    // Inizializza la variabile di condizione "not_full"
    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        perror("queue_init: Errore pthread_cond_init (not_full)");
        // Se fallisce l'init della seconda CV, dobbiamo distruggere la prima CV e il mutex
        pthread_cond_destroy(&q->not_empty); // Distrugge la CV precedentemente inizializzata
        pthread_mutex_destroy(&q->mutex);
        free(q->buffer);
        q->buffer = NULL;
        return -1; // Errore inizializzazione CV
    }

    // Se siamo arrivati qui, tutto è stato inizializzato correttamente
    printf("Coda condivisa inizializzata con successo (capacità %d).\n", capacity);
    return 0; // Successo
}

// Implementazione di queue_destroy
void queue_destroy(SharedQueue* q) {
    if (q == NULL) {
        return; // Non fare nulla se il puntatore è nullo
    }

    // È buona norma distruggere le risorse in ordine inverso rispetto all'inizializzazione

    // 1. Distruggi le variabili di condizione
    // Ignoriamo gli errori qui, anche se in produzione potremmo loggarli
    if (pthread_cond_destroy(&q->not_empty) != 0) {
         perror("queue_destroy: Errore pthread_cond_destroy (not_empty)");
    }
     if (pthread_cond_destroy(&q->not_full) != 0) {
         perror("queue_destroy: Errore pthread_cond_destroy (not_full)");
    }

    // 2. Distruggi il mutex
    if (pthread_mutex_destroy(&q->mutex) != 0) {
         perror("queue_destroy: Errore pthread_mutex_destroy");
    }

    // 3. Libera la memoria del buffer
    if (q->buffer != NULL) {
        free(q->buffer);
        q->buffer = NULL; // Buona pratica per evitare dangling pointers
    }

    // Opzionale: azzerare altri campi per sicurezza
    q->capacity = 0;
    q->head = 0;
    // ... etc ...

    printf("Coda condivisa distrutta.\n");
}

// --- IMPLEMENTAZIONE di push e pop (verrà fatta nel prossimo passo) ---
/**
 * @brief Inserisce un elemento nell coda condivisa
 * ACHTUNG!:
 * 1. Il Chiamante DEVE aver già acquisito il mutex q->mutex
 * 2. Il Chiamante DEVE aver già verificato che la coda non sia piena
 * QUESTA FUNZIONE NON ESEGUE CONTROLLI DI PIENO O LOCKING
 * 
 * @param q Puntatore alla SharedQueue
 * @param item L'elemento QueueItem da inserire
 */

void push(SharedQueue* q, QueueItem item) {
    //Inserisce l'elemento nella posizione 'tail' del buffer circolare
    q->buffer[q->tail] = item;

    //Aggiorna l'indice 'tail' per il prossimo inserimento, gestendo il wraparound
    q->tail = (q->tail + 1) % q->capacity;

    //Incrementa il contatore totale degli elementi presenti sulla coda
    q->count++;

    //Incrementa il contatore specifico per il tipo di elemento inserito
    if(item.type == ITEM_TYPE_WORK){
        q->work_items_count++;
    }else{
        q->results_in_queue_count++;
    }

    //(void)q; // Evita warning "unused parameter" per ora
    //(void)item; // Evita warning "unused parameter" per ora
    fprintf(stderr,"[DEBUG] push() chiamata\n"); // Debug temporaneo
}

// --- IMPLEMENTAZIONE di pop ---
/**
 * @brief Estrae un elemento dalla coda condivisa (implementazione effettiva).
 *
 * ASSUNZIONI (IMPORTANTE!):
 * 1. Il chiamante DEVE aver già acquisito il mutex q->mutex.
 * 2. Il chiamante DEVE aver già verificato che la coda non sia vuota
 * (tipicamente uscendo da un wait su q->not_empty).
 * QUESTA FUNZIONE NON ESEGUUE CONTROLLI DI VUOTO O LOCKING.
 *
 * @param q Puntatore alla SharedQueue.
 * @return L'elemento QueueItem estratto dalla testa della coda.
 */
QueueItem pop(SharedQueue* q) {
    // Preleva l'elemento dalla posizione 'head' del buffer circolare
    QueueItem item = q->buffer[q->head];

    // Opzionale: "Pulire" lo slot appena letto per evitare dati residui (utile per debug)
    // Potremmo usare memset o assegnare un valore "invalido" se ne avessimo uno
    // memset(&q->buffer[q->head], 0, sizeof(QueueItem)); // Esempio con memset

    // Aggiorna l'indice 'head' per la prossima estrazione, gestendo il wraparound
    q->head = (q->head + 1) % q->capacity;

    // Decrementa il contatore totale degli elementi presenti
    q->count--;

    // Decrementa il contatore specifico per il tipo di elemento estratto
    if (item.type == ITEM_TYPE_WORK) {
        q->work_items_count--;
    } else { // ITEM_TYPE_RESULT
        q->results_in_queue_count--;
    }

    // Stampa di Debug (Utile durante lo sviluppo, rimuovere in produzione)
    // fprintf(stderr, "[DEBUG] pop : count=%d, work=%d, results=%d, head=%d, tail=%d\n",
    //         q->count, q->work_items_count, q->results_in_queue_count, q->head, q->tail);

    // Restituisci l'elemento che era in testa
    return item;
}
