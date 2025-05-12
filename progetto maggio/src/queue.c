#include "queue.h" // Contiene già common.h e le definizioni necessarie

// Inizializza la coda
int init_queue(ConcurrentQueue *q) {
    DEBUG_PRINT_GEN("Inizializzazione coda...");
    q->head = q->tail = NULL;
    q->closed = 0;
    q->task_count = 0;
    // Inizializza il mutex richiesto per la sincronizzazione
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("Errore inizializzazione mutex coda");
        return -1;
    }
    // Inizializza la variabile di condizione richiesta per la sincronizzazione
    if (pthread_cond_init(&q->cond_non_empty, NULL) != 0) {
        perror("Errore inizializzazione cond var coda");
        pthread_mutex_destroy(&q->mutex); // Pulisci mutex già creato
        return -1;
    }
    DEBUG_PRINT_GEN("Coda inizializzata.");
    return 0;
}

// Distrugge la coda
void destroy_queue(ConcurrentQueue *q) {
    DEBUG_PRINT_GEN("Distruzione coda...");
    // Blocco il mutex per sicurezza, anche se idealmente non ci sono altri thread
    pthread_mutex_lock(&q->mutex);
    Node *current = q->head;
    Node *next;
    long freed_count = 0;
    // Libera tutti i nodi rimanenti nella lista
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
        freed_count++;
    }
    q->head = q->tail = NULL;
    DEBUG_PRINT_GEN("Liberati %ld nodi rimanenti.", freed_count);
    q->task_count = 0;
    pthread_mutex_unlock(&q->mutex); // Rilascia il mutex

    // Distrugge mutex e variabile di condizione
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_non_empty);
    DEBUG_PRINT_GEN("Coda distrutta.");
}

// Inserisce un task (operazione push)
void push(ConcurrentQueue *q, Partition_Index_Task task) {
    Node *newNode = malloc(sizeof(Node));
    CHECK_ERR(newNode == NULL, "Push: Errore allocazione nodo coda");
    newNode->task = task;
    newNode->next = NULL;

    // Acquisisce il lock per accesso esclusivo alla coda (richiesto da testo esame: mutex)
    pthread_mutex_lock(&q->mutex);

    // Aggiunge il nuovo nodo in coda alla lista
    if (q->tail == NULL) { // Se la coda era vuota
        q->head = q->tail = newNode;
    } else {
        q->tail->next = newNode;
        q->tail = newNode;
    }
    q->task_count++;

    // Segnala a UN thread in attesa su pop (se ce ne sono) che la coda non è più vuota
    // (richiesto da testo esame: variabili di condizione)
    pthread_cond_signal(&q->cond_non_empty);

    // Rilascia il lock
    pthread_mutex_unlock(&q->mutex);
}

// Estrae un task (operazione pop)
int pop(ConcurrentQueue *q, Partition_Index_Task *task) {
    // Acquisisce il lock per accesso esclusivo
    pthread_mutex_lock(&q->mutex);

    // Attende finché la coda non è vuota E non è chiusa
    // Testo esame: "...finché Q non è vuota..."
    while (q->head == NULL && !q->closed) {
        DEBUG_PRINT_GEN("POP: Coda vuota ma non chiusa. Attesa su cond_non_empty...");
        // Attesa condizionale: rilascia il mutex e aspetta un signal sulla cond var
        // (richiesto da testo esame: variabili di condizione)
        pthread_cond_wait(&q->cond_non_empty, &q->mutex);
        // Al risveglio, ri-acquisisce automaticamente il mutex e ricontrolla la condizione del while
        DEBUG_PRINT_GEN("POP: Risvegliato da cond_wait.");
    }

    // Se la coda è ancora vuota, significa che è stata chiusa (q->closed == 1)
    if (q->head == NULL) {
        pthread_mutex_unlock(&q->mutex); // Rilascia il lock prima di uscire
        return 0; // Segnala che non ci sono più task e non ne arriveranno
    }

    // Estrae il nodo dalla testa della coda
    Node *nodeToRemove = q->head;
    *task = nodeToRemove->task; // Copia il task nel puntatore fornito
    q->head = q->head->next;
    if (q->head == NULL) { // Se la coda è diventata vuota dopo il pop
        q->tail = NULL;
    }
    q->task_count--;

    // Rilascia il lock
    pthread_mutex_unlock(&q->mutex);

    free(nodeToRemove); // Libera la memoria del nodo estratto
    return 1; // Segnala che un task è stato estratto con successo
}

// Chiude la coda
void close_queue(ConcurrentQueue *q) {
    // Acquisisce il lock
    pthread_mutex_lock(&q->mutex);
    if (!q->closed) {
        q->closed = 1;
        DEBUG_PRINT_GEN("CLOSE_QUEUE: Flag 'closed' impostato.");
        // Sveglia TUTTI i thread potenzialmente in attesa su cond_wait.
        // Questo assicura che vedano q->closed == 1 e terminino se q->head == NULL.
        pthread_cond_broadcast(&q->cond_non_empty);
    }
    // Rilascia il lock
    pthread_mutex_unlock(&q->mutex);
}