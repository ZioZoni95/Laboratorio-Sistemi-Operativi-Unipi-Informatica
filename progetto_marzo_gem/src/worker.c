// File: src/worker.c
#include "common.h" // Include stdio, stdlib, pthread, bool, structs
#include "queue.h"  // Include i prototipi di push/pop
#include "worker.h" // Include il prototipo di worker_routine e calculate_sum

extern long *V;
// --- Implementazione Funzione Helper ---
// Calcola la somma degli elementi in un WorkItem
long long calculate_sum(WorkItem work) {
    long long sum = 0;
    if (work.data_ptr == NULL || work.num_elements <= 0) {
        return 0; // Caso anomalo o gruppo vuoto
    }
    // printf("[Worker DEBUG] Calcolo somma per %d elementi a partire da %p\n", work.num_elements, work.data_ptr);
    for (int i = 0; i < work.num_elements; ++i) {
        sum += work.data_ptr[i];
    }
    return sum;
}

// --- Implementazione Funzione Worker ---
void* worker_routine(void* arg) {
    // Recupera il puntatore alla coda condivisa
    SharedQueue* Q = (SharedQueue*) arg;
    long thread_id = (long)pthread_self(); // Ottieni ID thread per logging

    printf("[Worker %lx] Avviato.\n", thread_id);

    long long my_partial_sum = 0; // Somma parziale locale di questo worker

    // --- FASE 1, 2, 3: Ciclo Consumo Lavoro ---
    // Flusso: Continua a ciclare cercando lavoro finché il Master non ha finito
    // E non ci sono più elementi di lavoro nella coda.
    while (true) {
        // --- Sezione Critica: Controllo/Estrazione Lavoro ---
        pthread_mutex_lock(&Q->mutex);

        // Attendi se non c'è LAVORO E il Master non ha ANCORA finito (Controllo MESA)
        while (Q->work_items_count == 0 && !Q->master_done_producing) {
            // printf("[Worker %lx] Coda vuota di lavoro (%d) e Master attivo. Attendo...\n", thread_id, Q->work_items_count);
            pthread_cond_wait(&Q->not_empty, &Q->mutex);
            // printf("[Worker %lx] Risvegliato da attesa not_empty.\n", thread_id);
        }

        // Stato dopo il wait: O c'è lavoro O il master ha finito O entrambi

        WorkItem work_to_do; // Variabile locale temporanea
        bool should_break = false; // Flag per uscire dal ciclo dopo unlock

        if (Q->work_items_count > 0) {
            // C'è lavoro, PRENDILO
            QueueItem item = pop(Q); // pop è definita in queue.c
            work_to_do = item.data.work;
            // printf("[Worker %lx] Lavoro prelevato. count=%d, work_items=%d\n", thread_id, Q->count, Q->work_items_count);

            
            // Segnala spazio libero (potrebbe servire al Master o ad altri Worker per risultati)
            pthread_cond_signal(&Q->not_full);
            pthread_mutex_unlock(&Q->mutex); // Rilascia lock PRIMA di elaborare

            // <<< NUOVA STAMPA 1 - DOPO IL POP E L'UNLOCK >>>
            // Calcola gli indici relativi all'inizio di V per il log
            long start_index = work_to_do.data_ptr - V;
            long end_index = start_index + work_to_do.num_elements - 1;
            printf("[Worker %lx] Ricevuto gruppo: Indici %ld - %ld (%d elem)\n",
                   thread_id, start_index, end_index, work_to_do.num_elements);


            // --- Elaborazione Lavoro (FUORI da mutex) ---
            // printf("[Worker %lx] Elaboro gruppo...\n", thread_id);
            long long sum_group = calculate_sum(work_to_do);
            // <<< NUOVA STAMPA 2 - DOPO IL CALCOLO >>>
            printf("[Worker %lx] Somma per gruppo [%ld-%ld]: %lld\n",
                thread_id, start_index, end_index, sum_group);
                
            my_partial_sum += sum_group;
            // printf("[Worker %lx] Gruppo elaborato. Somma gruppo: %lld, Somma parziale: %lld\n", thread_id, sum_group, my_partial_sum);

            // Continua il ciclo per cercare altro lavoro (non impostare should_break)

        } else if (Q->master_done_producing) {
            // Se siamo qui: NO lavoro in coda (Q->work_items_count == 0)
            // E Master HA FINITO (Q->master_done_producing == true)
            // -> Il worker deve smettere di cercare lavoro.
            // printf("[Worker %lx] Non c'è più lavoro e Master ha finito. Esco dal ciclo.\n", thread_id);
            should_break = true; // Imposta il flag per uscire dopo unlock
            pthread_mutex_unlock(&Q->mutex); // Rilascia lock
        } else {
            // Sveglia spuria o stato transitorio (improbabile ma possibile)
            // Semplicemente rilascia il lock e riprova il ciclo
             // printf("[Worker %lx] Sveglia spuria? Riprovo.\n", thread_id);
            pthread_mutex_unlock(&Q->mutex);
            // Non impostare should_break
        }
        // --- Fine Sezione Critica (o quasi, l'unlock è sopra) ---

        if (should_break) {
            break; // Esci dal ciclo while(true)
        }
        // Altrimenti (se abbiamo processato lavoro o avuto sveglia spuria), il ciclo continua
    } // Fine ciclo while(true)

    // --- FASE 4: Inserimento Risultato in Q ---
    // Flusso: Crea l'item risultato, attendi spazio in coda se necessario, inserisci.
    printf("[Worker %lx] Invio risultato parziale: %lld\n", thread_id, my_partial_sum);
    ResultItem result;
    result.partial_sum = my_partial_sum;
    QueueItem item_to_push;
    item_to_push.type = ITEM_TYPE_RESULT;
    item_to_push.data.result = result;

    // --- Sezione Critica: Inserimento Risultato ---
    pthread_mutex_lock(&Q->mutex);

    // Attendi se Q è piena (Controllo MESA)
    while (Q->count == Q->capacity) {
        // printf("[Worker %lx] Coda piena (%d/%d). Attendo per inserire risultato...\n", thread_id, Q->count, Q->capacity);
        pthread_cond_wait(&Q->not_full, &Q->mutex);
        // printf("[Worker %lx] Risvegliato da attesa not_full (per risultato).\n", thread_id);
    }

    // Inserisci il risultato (push è definita in queue.c)
    push(Q, item_to_push);
    // printf("[Worker %lx] Risultato inserito. count=%d, results_in_queue=%d\n", thread_id, Q->count, Q->results_in_queue_count);

    // Segnala che Q non è vuota (SVEGLIA IL MASTER!)
    pthread_cond_signal(&Q->not_empty);

    pthread_mutex_unlock(&Q->mutex);
    // --- Fine Sezione Critica ---

    // --- FASE 5: Termina ---
    printf("[Worker %lx] Terminato.\n", thread_id);
    return NULL; // Fine esecuzione Worker
}