// File: src/master.c
#include "common2.h" // Include externs V, N, k, SharedQueue, ecc.
#include "queue2.h"  // Include funzioni coda concorrente
#include "master2.h"

// Definizione min (spesso è utile metterla in un utils.c/h)
int min(int a, int b) { return (a < b) ? a : b; }

// Funzione Master eseguita nel thread main
void* master_routine(void* arg) {
    // L'argomento passato da main è il puntatore alla coda
    SharedQueue* Q = (SharedQueue*) arg;
    // Otteniamo il numero di worker dalla coda stessa
    int num_workers = Q->num_workers;

    printf("[Master] Avviato (nel thread main).\n");

    int current_index = 0;

    // --- FASE 1 & 2: Produzione Gruppi e Enqueue ---
    printf("[Master] Inizio produzione task (N=%d, k=%d)...\n", N, k);
    int groups_sent = 0;
    while (current_index < N) {
        // Calcola quanti elementi mettere nel prossimo gruppo (multiplo di 2, max 2*k)
        int elements_remaining = N - current_index;
        int elements_this_group = min(2 * k, elements_remaining);

        // Assicurati che sia almeno una coppia (2 elementi)
        if (elements_this_group < 2) {
            if (elements_remaining > 0) {
                printf("[Master] Avanzato %d elemento/i, non sufficienti per una coppia. Ignoro.\n", elements_remaining);
            }
            break; // Non ci sono abbastanza elementi per formare un'altra coppia
        }
        // Arrotonda per difetto al multiplo di 2 più vicino se necessario
        // (anche se min(2*k, ...) dovrebbe già dare un multiplo di 2 se N è pari o k è gestito bene)
        // Per sicurezza:
        elements_this_group = (elements_this_group / 2) * 2;
        if (elements_this_group == 0) { // Se k=0 o N<2 inizialmente
             break;
        }


        // Crea il WorkItem
        WorkItem work;
        work.data_ptr = &V[current_index]; // Usa V globale (dichiarato extern)
        work.num_elements = elements_this_group;

        QueueItem item_to_push;
        item_to_push.type = ITEM_TYPE_WORK;
        item_to_push.data.work = work;

        printf("[Master] Preparato gruppo %d: Indici %d - %d (%d elem)\n",
               groups_sent, current_index, current_index + elements_this_group - 1, elements_this_group);

        // Inserisci nella coda concorrente (si blocca se piena)
        if (!queue_enqueue(Q, item_to_push)) {
            // Questo succede se la coda segnala shutdown mentre tentiamo di inserire
            printf("[Master] Enqueue fallito (coda in shutdown?). Interrompo produzione.\n");
            break;
        }
        // printf("[Master] Gruppo %d inserito nella coda.\n", groups_sent);
        groups_sent++;
        current_index += elements_this_group;
    }
    printf("[Master] Fine produzione. %d gruppi inviati.\n", groups_sent);

    // --- FASE 3: Notifica Fine Produzione ---
    printf("[Master] Notifico fine produzione ai worker...\n");
    queue_signal_master_done(Q); // Usa la funzione helper della coda

    // --- FASE 4: Raccolta Risultati ---
    printf("[Master] Inizio raccolta %d risultati...\n", num_workers);
    long long final_total_sum = 0;
    int collected_results = 0;

    while (collected_results < num_workers) {
        QueueItem received_item;
        // Usa Dequeue CONCORRENTE (attende se vuota)
        if (queue_dequeue(Q, &received_item)) {
            // Elemento ricevuto. Ci aspettiamo sia un ResultItem.
            if (received_item.type == ITEM_TYPE_RESULT) {
                final_total_sum += received_item.data.result.partial_sum;
                collected_results++;
                printf("[Master] ---> Ricevuto risultato %d/%d. Somma parziale worker: %lld (Aggregata: %lld)\n",
                       collected_results, num_workers,
                       received_item.data.result.partial_sum, final_total_sum);
            } else {
                // Se riceviamo un WorkItem qui, c'è un errore o un worker non ha finito
                fprintf(stderr, "[Master] ERRORE: Ricevuto WorkItem (%d elementi da %p) durante la raccolta risultati!\n",
                        received_item.data.work.num_elements, received_item.data.work.data_ptr);
                // Cosa fare? Ignorare? Rimettere in coda? Segnalare errore?
                // Per ora lo segnaliamo e basta.
            }
        } else {
            // Dequeue ha fallito -> shutdown segnalato e coda vuota.
            // Se non abbiamo ancora tutti i risultati, qualche worker potrebbe
            // essere terminato in modo anomalo senza inviare il risultato.
            fprintf(stderr, "[Master] ERRORE: Dequeue fallito prima di raccogliere tutti i risultati (%d/%d).\n",
                    collected_results, num_workers);
            break; // Esce dal loop di raccolta
        }
    }
    if(collected_results == num_workers) {
        printf("[Master] Raccolti tutti i %d risultati.\n", num_workers);
    } else {
         printf("[Master] Attenzione: Raccolti solo %d risultati su %d attesi.\n", collected_results, num_workers);
    }


    // --- FASE 5: Stampa Risultato Finale ---
    // Questo viene eseguito alla fine della logica del master, prima che main faccia join/cleanup
    printf("\n------------------------------------------\n");
    printf("[Master] SOMMA FINALE CALCOLATA: %lld\n", final_total_sum);
    printf("------------------------------------------\n");

    printf("[Master] Routine terminata.\n");
    // Non c'è return esplicito perché master_routine è chiamata da main,
    // ma se fosse un thread separato, faremmo:
    // return NULL;
    return NULL; // Aggiunto per coerenza con firma void*
}