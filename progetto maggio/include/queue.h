#ifndef QUEUE_H
#define QUEUE_H

#include "common.h" // Include Task, ThreadArgs, etc.

// Nodo della lista linkata usata per implementare la coda.
typedef struct Node {
    Partition_Index_Task task;    // Il task (start, end) contenuto nel nodo
    struct Node *next;  // Puntatore al nodo successivo nella coda
} Node;

// Struttura Coda Concorrente (Q nel testo d'esame).
// Le operazioni push e pop sono thread-safe.
struct ConcurrentQueue {
    Node *head;             
    Node *tail;             
    pthread_mutex_t mutex;  // Mutex per garantire accesso esclusivo alla coda
    pthread_cond_t cond_non_empty; // Variabile di condizione per segnalare quando la coda non è vuota
    int closed;             // Flag: 1 se non verranno aggiunti più task iniziali, 0 altrimenti
    long task_count;        // Numero di task attualmente nella coda (per debug/info)
};

// --- Dichiarazioni Funzioni Coda ---

// Inizializza la coda concorrente.
// Deve essere chiamata prima di usare la coda.
// Restituisce 0 in caso di successo, -1 in caso di errore.
int init_queue(ConcurrentQueue *q);

// Distrugge la coda concorrente.
// Libera la memoria dei nodi rimanenti e distrugge mutex/cond var.
// Deve essere chiamata quando la coda non è più necessaria.
void destroy_queue(ConcurrentQueue *q);

// Inserisce un task (push) in fondo alla coda in modo thread-safe.
// Corrisponde all'operazione 'push' menzionata nel testo d'esame.
void push(ConcurrentQueue *q, Partition_Index_Task task);

// Estrae un task (pop) dalla testa della coda in modo thread-safe.
// Se la coda è vuota, attende finché non arriva un task o la coda viene chiusa.
// Corrisponde all'operazione 'pop' menzionata nel testo d'esame.
// Restituisce 1 se un task è stato estratto con successo (e copiato in 'task').
// Restituisce 0 se la coda è vuota E chiusa (non arriveranno altri task).
int pop(ConcurrentQueue *q, Partition_Index_Task *task);

// Segnala che non verranno più aggiunti task iniziali alla coda.
// Usato da Worker 0 dopo aver inserito tutte le partizioni iniziali.
// Permette ai worker in attesa su pop di terminare se la coda diventa vuota.
void close_queue(ConcurrentQueue *q);

#endif // QUEUE_H