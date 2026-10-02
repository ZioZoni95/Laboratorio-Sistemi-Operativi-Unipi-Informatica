/**
 * @file queue.c
 * @brief Implementazione di una coda concorrente (thread-safe) per task di partizioni.
 *
 * La coda è implementata come una lista linkata singolarmente.
 * Le operazioni `push` e `pop` sono rese thread-safe utilizzando un mutex (`q->mutex`)
 * per l'accesso esclusivo alla struttura dati della coda e una variabile di condizione
 * (`q->cond_non_empty`) per gestire l'attesa dei thread consumatori (pop) quando
 * la coda è vuota.
 * La coda può essere "chiusa" (`q->closed = 1`), segnalando che non verranno più
 * aggiunti nuovi task. Questo permette ai thread consumatori di terminare
 * correttamente quando la coda è vuota e chiusa.
 */

#include "queue.h" 
#include <stdio.h>  // Per perror (anche se CHECK_ERR potrebbe già usarlo)
#include <stdlib.h> // Per malloc, free

/**
 * @brief Inizializza una coda concorrente.
 *
 * Imposta head e tail a NULL, closed a 0, task_count a 0.
 * Inizializza il mutex e la variabile di condizione associati alla coda.
 *
 * @param q Puntatore alla struttura ConcurrentQueue da inizializzare.
 * @return 0 in caso di successo, -1 in caso di fallimento nell'inizializzazione
 * del mutex o della variabile di condizione (con messaggio di errore stampato).
 */
int init_queue(ConcurrentQueue *q) {
    DEBUG_PRINT_GEN("Inizializzazione coda...");
    q->head = q->tail = NULL; // Coda inizialmente vuota
    q->closed = 0;            // Coda inizialmente aperta
    q->task_count = 0;        // Nessun task presente

    // Inizializza il mutex per la sincronizzazione dell'accesso alla coda
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("Errore inizializzazione mutex coda"); // Stampa errore di sistema
        return -1; // Fallimento
    }
    // Inizializza la variabile di condizione usata per segnalare che la coda non è vuota
    if (pthread_cond_init(&q->cond_non_empty, NULL) != 0) {
        perror("Errore inizializzazione cond var coda");
        pthread_mutex_destroy(&q->mutex); // Pulisce il mutex già creato prima di fallire
        return -1; // Fallimento
    }
    DEBUG_PRINT_GEN("Coda inizializzata.");
    return 0; // Successo
}

/**
 * @brief Distrugge una coda concorrente.
 *
 * Libera la memoria di tutti i nodi rimanenti nella coda.
 * Distrugge il mutex e la variabile di condizione associati.
 * È importante che nessun thread stia usando la coda quando questa funzione viene chiamata.
 *
 * @param q Puntatore alla struttura ConcurrentQueue da distruggere.
 */
void destroy_queue(ConcurrentQueue *q) {
    DEBUG_PRINT_GEN("Distruzione coda...");
    // Blocca il mutex per assicurare l'accesso esclusivo durante la distruzione,
    // sebbene idealmente non ci dovrebbero essere altri thread attivi sulla coda.
    pthread_mutex_lock(&q->mutex);

    Node *current = q->head; // Puntatore per scorrere la lista dei nodi
    Node *next;
    long freed_count = 0; // Contatore dei nodi liberati (per debug)

    // Libera tutti i nodi rimanenti nella lista
    while (current != NULL) {
        next = current->next; // Salva il puntatore al nodo successivo
        free(current);        // Libera il nodo corrente
        current = next;       // Passa al nodo successivo
        freed_count++;
    }
    q->head = q->tail = NULL; // Resetta head e tail
    DEBUG_PRINT_GEN("Liberati %ld nodi rimanenti.", freed_count);
    q->task_count = 0; // Resetta il contatore dei task

    pthread_mutex_unlock(&q->mutex); // Rilascia il mutex

    // Distrugge il mutex e la variabile di condizione
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_non_empty);
    DEBUG_PRINT_GEN("Coda distrutta.");
}

/**
 * @brief Inserisce un task (operazione push) in fondo alla coda in modo thread-safe.
 *
 * Alloca un nuovo nodo, vi copia il task, e lo aggiunge alla fine della lista (tail).
 * Se la coda era vuota, head e tail punteranno al nuovo nodo.
 * Segnala (pthread_cond_signal) alla variabile di condizione `cond_non_empty`
 * per svegliare un eventuale thread in attesa su `pop`.
 *
 * @param q Puntatore alla ConcurrentQueue.
 * @param task Il Partition_Index_Task da inserire.
 */
void push(ConcurrentQueue *q, Partition_Index_Task task) {
    // Alloca memoria per un nuovo nodo della lista
    Node *newNode = malloc(sizeof(Node));
    CHECK_ERR(newNode == NULL, "Push: Errore allocazione nodo coda"); // Controlla fallimento malloc
    newNode->task = task; // Copia il task nel nodo
    newNode->next = NULL; // Il nuovo nodo è l'ultimo, quindi next è NULL

    // Acquisisce il lock per l'accesso esclusivo alla coda
    pthread_mutex_lock(&q->mutex);

    // Aggiunge il nuovo nodo in coda alla lista
    if (q->tail == NULL) { // Se la coda era vuota
        q->head = q->tail = newNode; // Head e Tail puntano al nuovo (unico) nodo
    } else { // Se la coda non era vuota
        q->tail->next = newNode; // Il vecchio tail punta al nuovo nodo
        q->tail = newNode;       // Il nuovo nodo diventa il nuovo tail
    }
    q->task_count++; // Incrementa il contatore dei task

    // Segnala a UN thread in attesa su pop (se ce ne sono) che la coda non è più vuota.
    // Questo risveglierà un thread bloccato in pthread_cond_wait su q->cond_non_empty.
    pthread_cond_signal(&q->cond_non_empty);

    // Rilascia il lock
    pthread_mutex_unlock(&q->mutex);
}

/**
 * @brief Estrae un task (operazione pop) dalla testa della coda in modo thread-safe.
 *
 * Se la coda è vuota e non è chiusa (`q->closed == 0`), il thread chiamante
 * si mette in attesa sulla variabile di condizione `q->cond_non_empty` finché
 * un task non viene inserito o la coda non viene chiusa.
 * Se la coda è vuota e chiusa (`q->closed == 1`), restituisce 0 (nessun task).
 * Altrimenti, rimuove il nodo dalla testa (head), copia il task, libera il nodo,
 * e restituisce 1.
 *
 * @param q Puntatore alla ConcurrentQueue.
 * @param task Puntatore a Partition_Index_Task dove verrà copiato il task estratto.
 * @return 1 se un task è stato estratto con successo e copiato in `*task`.
 * @return 0 se la coda è vuota E chiusa (quindi non arriveranno altri task).
 */
int pop(ConcurrentQueue *q, Partition_Index_Task *task) {
    // Acquisisce il lock per l'accesso esclusivo alla coda
    pthread_mutex_lock(&q->mutex);

    // Attende finché la coda è vuota E non è ancora stata chiusa.
    // Questo ciclo `while` è cruciale per gestire "spurious wakeups" di pthread_cond_wait.
    while (q->head == NULL && !q->closed) {
        DEBUG_PRINT_GEN("POP: Coda vuota ma non chiusa. Attesa su cond_non_empty...");
        // Attesa condizionale: rilascia atomicamente il mutex `q->mutex` e si mette in attesa
        // che `q->cond_non_empty` venga segnalata. Quando risvegliato, ri-acquisisce
        // automaticamente il mutex prima di continuare.
        pthread_cond_wait(&q->cond_non_empty, &q->mutex);
        // Al risveglio, il thread ricontrolla la condizione del while.
        DEBUG_PRINT_GEN("POP: Risvegliato da cond_wait.");
    }

    // Se la coda è ancora vuota a questo punto, significa che deve essere stata chiusa
    // (altrimenti il while sopra non sarebbe terminato se q->head è NULL).
    if (q->head == NULL) { // Implica q->closed == 1
        pthread_mutex_unlock(&q->mutex); // Rilascia il lock prima di uscire
        return 0; // Segnala che non ci sono più task e non ne arriveranno (coda vuota e chiusa)
    }

    // Estrae il nodo dalla testa della coda
    Node *nodeToRemove = q->head;    // Nodo da rimuovere
    *task = nodeToRemove->task;      // Copia il task nel puntatore fornito dal chiamante
    q->head = q->head->next;         // Avanza head al nodo successivo
    if (q->head == NULL) {           // Se la coda è diventata vuota dopo il pop
        q->tail = NULL;              // Anche tail diventa NULL
    }
    q->task_count--; // Decrementa il contatore dei task

    // Rilascia il lock
    pthread_mutex_unlock(&q->mutex);

    free(nodeToRemove); // Libera la memoria del nodo estratto (dopo aver rilasciato il mutex)
    return 1; // Segnala che un task è stato estratto con successo
}

/**
 * @brief Chiude la coda, segnalando che non verranno più aggiunti task.
 *
 * Imposta il flag `q->closed` a 1.
 * Esegue un `pthread_cond_broadcast` su `q->cond_non_empty` per svegliare TUTTI
 * i thread che potrebbero essere in attesa su `pop`. Questo è importante
 * per assicurare che tutti i consumer vedano che la coda è stata chiusa e
 * possano terminare se la coda è anche vuota.
 *
 * @param q Puntatore alla ConcurrentQueue.
 */
void close_queue(ConcurrentQueue *q) {
    // Acquisisce il lock
    pthread_mutex_lock(&q->mutex);
    if (!q->closed) { // Esegue l'operazione solo se la coda non è già chiusa
        q->closed = 1; // Imposta il flag 'closed'
        DEBUG_PRINT_GEN("CLOSE_QUEUE: Flag 'closed' impostato.");
        // Sveglia TUTTI i thread potenzialmente in attesa su pthread_cond_wait.
        // Questo assicura che i thread controllino nuovamente la condizione (q->head == NULL && !q->closed)
        // e, vedendo q->closed == 1, escano dal loop di attesa se q->head è NULL.
        pthread_cond_broadcast(&q->cond_non_empty);
    }
    // Rilascia il lock
    pthread_mutex_unlock(&q->mutex);
}