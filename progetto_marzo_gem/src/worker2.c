// File: src/worker.c
#include "common2.h" // Include WorkerThreadArgs, SharedQueue, ecc.
#include "queue2.h"  // Include funzioni coda concorrente
#include "worker2.h"

// --- Funzione Helper ---
// Calcola la somma degli elementi usando il puntatore e il count dal WorkItem
long long calculate_sum(WorkItem work) {
    long long sum = 0;
    if (work.data_ptr == NULL || work.num_elements <= 0) {
        fprintf(stderr, "[Worker Calc] Ricevuto WorkItem invalido (ptr=%p, count=%d)\n",
                (void*)work.data_ptr, work.num_elements);
        return 0; // Gestione caso anomalo
    }
    for (int i = 0; i < work.num_elements; ++i) {
        sum += work.data_ptr[i]; // Accede agli elementi dell'array V tramite il puntatore
    }
    return sum;
}

// --- Funzione Worker ---
void* worker_routine(void* arg) {
    // Recupera argomenti e risorse dalla struct passata
    WorkerThreadArgs* args = (WorkerThreadArgs*) arg;
    SharedQueue* Q = args->queue_ptr;
    int worker_id = args->worker_id;
    // long* V_base = args->v_base_ptr; // Disponibile per calcoli di indice, se servisse

    printf("[Worker %d] Avviato (TID: %lx).\n", worker_id, (unsigned long)pthread_self());

    long long my_partial_sum = 0; // Accumulatore locale per questo worker
    int groups_processed = 0;     // Contatore locale per i gruppi processati

    // --- Ciclo Consumo Lavoro ---
    while (true) {
        QueueItem received_item;

        // --- Usa Dequeue CONCORRENTE ---
        // Attende se vuota, ritorna false quando master_done_producing è true E la coda è vuota.
        if (queue_dequeue(Q, &received_item)) {
            // Elemento ricevuto. Ci aspettiamo sia un WorkItem.
            if (received_item.type == ITEM_TYPE_WORK) {
                WorkItem work_to_do = received_item.data.work;

                // Calcola gli indici solo per un log più informativo
                long start_index = work_to_do.data_ptr - args->v_base_ptr;
                long end_index = start_index + work_to_do.num_elements - 1;
                printf("[Worker %d] Ricevuto gruppo: Indici %ld - %ld (%d elem)\n",
                       worker_id, start_index, end_index, work_to_do.num_elements);

                // Elaborazione Lavoro (chiamata a funzione helper)
                long long sum_group = calculate_sum(work_to_do);
                printf("[Worker %d] -> Somma gruppo [%ld-%ld]: %lld\n",
                       worker_id, start_index, end_index, sum_group);

                // Aggiorna somma parziale locale
                my_partial_sum += sum_group;
                groups_processed++;

            } else {
                // Errore inaspettato: Worker ha ricevuto un ResultItem dalla coda?
                fprintf(stderr, "[Worker %d] ERRORE: Ricevuto ResultItem invece di WorkItem dalla coda!\n", worker_id);
                // Potrebbe indicare un errore nella logica del Master o nella gestione della coda.
                // Per robustezza, si potrebbe provare a rimetterlo in coda (difficile!) o ignorarlo.
                // Ignorandolo, il master potrebbe bloccarsi aspettando un risultato mancante.
            }
        } else {
            // Dequeue ha restituito false -> Fine lavoro
            printf("[Worker %d] Fine lavoro segnalata dalla coda. Uscita dal loop.\n", worker_id);
            break; // Esce dal ciclo while(true)
        }
    } // Fine ciclo while(true)

    // --- Invio Risultato Finale del Worker ---
    printf("[Worker %d] Invio risultato parziale finale: %lld (%d gruppi processati)\n", worker_id, my_partial_sum, groups_processed);
    ResultItem result;
    result.partial_sum = my_partial_sum;
    QueueItem item_to_push;
    item_to_push.type = ITEM_TYPE_RESULT;
    item_to_push.data.result = result;

    // --- Usa Enqueue CONCORRENTE ---
    // Si blocca se la coda è piena (improbabile se C >= W)
    if (!queue_enqueue(Q, item_to_push)) {
         // Enqueue può fallire se master_done_producing è già true E la coda è piena.
         fprintf(stderr, "[Worker %d] ERRORE: Enqueue del risultato finale fallito!\n", worker_id);
         // Il risultato di questo worker andrà perso.
    } else {
        // printf("[Worker %d] Risultato finale inserito nella coda.\n", worker_id);
    }

    printf("[Worker %d] Terminato.\n", worker_id);
    return NULL; // Fine esecuzione Worker
}