// Definizione della coda condivisa Q
struct Queue {
    Gruppo gruppi[C];  // Array circolare
    int in, out, count;
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_full, cond_not_empty;
};

// Funzione per inserire un gruppo nella coda
void queue_push(Queue* q, Gruppo g) {
    lock(mutex);
    while (q->count == C) wait(cond_not_full);
    q->gruppi[q->in] = g;
    q->in = (q->in + 1) % C;
    q->count++;
    signal(cond_not_empty);
    unlock(mutex);
}

// Funzione per prelevare un gruppo dalla coda
Gruppo queue_pop(Queue* q) {
    lock(mutex);
    while (q->count == 0 && !production_finished) wait(cond_not_empty);
    if (q->count == 0) { unlock(mutex); return NULL; }  // Uscita se produzione terminata
    Gruppo g = q->gruppi[q->out];
    q->out = (q->out + 1) % C;
    q->count--;
    signal(cond_not_full);
    unlock(mutex);
    return g;
}

// THREAD MASTER
void* thread_master(void* arg) {
    Dividi array in gruppi di coppie (max k coppie per gruppo);
    Per ogni gruppo {
        queue_push(Q, gruppo);
    }
    production_finished = 1;
    Signal(cond_not_empty) per svegliare i worker;

    // Raccolta risultati dai worker
    int risultato_finale = 0;
    for (i = 0; i < num_worker; i++) {
        int* somma_parziale = queue_pop(Q);
        risultato_finale += *somma_parziale;
    }
    Stampa(risultato_finale);
}

// THREAD WORKER
void* thread_worker(void* arg) {
    int somma_parziale = 0;
    while (true) {
        Gruppo g = queue_pop(Q);
        if (g == NULL) break;  // Se produzione terminata, esco

        Per ogni coppia in g {
            somma_parziale += somma della coppia;
        }
    }
    queue_push(Q, &somma_parziale);
}
