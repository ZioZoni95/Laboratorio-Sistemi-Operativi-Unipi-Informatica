// File: src/master.c
#include "common.h" // Include stdio, stdlib, pthread, bool, structs
#include "queue.h"  // Include i prototipi di push/pop
#include "master.h" // Include il prototipo di master_routine e min

// Definizione della funzione min (se non in utils.c)
int min(int a, int b) {
    return (a < b) ? a : b;
}

// Variabili globali definite in main.c (usiamo extern per accedervi)
// Alternativa: passare una struct più grande come argomento a master_routine
extern long* V;
extern int N;
extern int k;
// Notare che Q è passato come argomento, W è in Q->num_workers

// --- Implementazione Funzione Master ---
void* master_routine(void* arg) {
    // Cast dell'argomento void* al tipo corretto
    SharedQueue* Q = (SharedQueue*) arg;

    printf("[Master] Avviato.\n");

    int current_index = 0; // Indice per scorrere l'array V

    // --- FASE 1 & 2: Produzione Gruppi e Inserimento in Q ---
    // Flusso: Cicla sull'array V, crea gruppi, li mette in coda finché V non è finito.
    printf("[Master] Inizio produzione gruppi...\n");
    while (current_index < N) {
        // Calcola dimensione gruppo (da 1 a k coppie -> 2 a 2*k elementi)
        int elements_in_group = min(2 * k, N - current_index);
        // Gestione caso limite: se rimangono meno di 2 elementi, non formano una coppia.
        // Potremmo decidere di ignorarli o processarli comunque.
        // Per ora, se < 2 (cioè 0 o 1), fermiamoci o gestiamolo come gruppo speciale.
        // Semplificazione: assumiamo che N sia tale da non lasciare elementi singoli isolati
        // o che il worker possa gestire num_elements=1 se necessario.
        if (elements_in_group <= 0) { // Modificato per sicurezza se N è piccolo
             break;
        }


        // Crea WorkItem
        WorkItem work;
        work.data_ptr = &V[current_index];
        work.num_elements = elements_in_group;
        QueueItem item_to_push;
        item_to_push.type = ITEM_TYPE_WORK;
        item_to_push.data.work = work;

        // <<< NUOVA STAMPA - PRIMA DEL LOCK >>>
        // Stampa gli indici del gruppo creato (calcolati rispetto a V)
        printf("[Master] Creato gruppo: Indici %d - %d (%d elem)\n",
               current_index,
               current_index + elements_in_group - 1,
               elements_in_group);

        // --- Sezione Critica: Inserimento in Coda ---
        // printf("[Master] Tento di inserire gruppo da indice %d (%d elem)\n", current_index, elements_in_group);
        pthread_mutex_lock(&Q->mutex);

        // Attendi se Q è piena (Controllo MESA)
        while (Q->count == Q->capacity) {
            printf("[Master] Coda piena (%d/%d). Attendo...\n", Q->count, Q->capacity);
            pthread_cond_wait(&Q->not_full, &Q->mutex);
            printf("[Master] Risvegliato da attesa not_full.\n");
        }

        // Inserisci in Q (push è definita in queue.c)
        push(Q, item_to_push);
        printf("[Master] Gruppo inserito. count=%d, work_items=%d\n", Q->count, Q->work_items_count);

        // Segnala che Q non è vuota a UN worker in attesa
        pthread_cond_signal(&Q->not_empty);

        pthread_mutex_unlock(&Q->mutex);
        // --- Fine Sezione Critica ---

        current_index += elements_in_group;
    } // Fine ciclo produzione
    printf("[Master] Fine produzione gruppi.\n");

    // --- FASE 3: Notifica Fine Produzione ---
    // Flusso: Imposta il flag e sveglia tutti i worker.
    printf("[Master] Notifico fine produzione ai worker...\n");
    pthread_mutex_lock(&Q->mutex);
    Q->master_done_producing = true;
    // SVEGLIA TUTTI i worker in attesa su not_empty
    pthread_cond_broadcast(&Q->not_empty);
    pthread_mutex_unlock(&Q->mutex);

    // --- FASE 4: Raccolta Risultati da Q ---
    // Flusso: Cicla finché non ha ricevuto un risultato per ogni worker,
    // attendendo passivamente se non ci sono risultati disponibili.
    printf("[Master] Inizio raccolta %d risultati...\n", Q->num_workers);
    long long final_total_sum = 0;
    int collected_results = 0;

    while (collected_results < Q->num_workers) {
        // --- Sezione Critica: Estrazione Risultato ---
        pthread_mutex_lock(&Q->mutex);

        // Attendi se non ci sono RISULTATI in Q (Controllo MESA)
        while (Q->results_in_queue_count == 0) {
            // printf("[Master] Nessun risultato pronto (%d/%d). Attendo...\n", Q->results_in_queue_count, Q->num_workers - collected_results);
            pthread_cond_wait(&Q->not_empty, &Q->mutex);
            // printf("[Master] Risvegliato da attesa not_empty (per risultati).\n");
        }

        // Estrai da Q (pop è definita in queue.c)
        QueueItem received_item = pop(Q);
        // printf("[Master] Risultato estratto. count=%d, results_in_queue=%d\n", Q->count, Q->results_in_queue_count);

        // Segnala che si è liberato spazio
        pthread_cond_signal(&Q->not_full);

        pthread_mutex_unlock(&Q->mutex);
        // --- Fine Sezione Critica ---

        // Processa l'item DOPO aver rilasciato il lock
        if (received_item.type == ITEM_TYPE_RESULT) {
            final_total_sum += received_item.data.result.partial_sum;
            collected_results++;
            printf("[Master] Ricevuto risultato %d/%d. Somma parziale: %lld\n",
                   collected_results, Q->num_workers, received_item.data.result.partial_sum);
        } else {
            // Questo non dovrebbe succedere con la logica attuale
            fprintf(stderr, "[Master] ERRORE: Ricevuto WorkItem invece di ResultItem!\n");
        }
    } // Fine ciclo raccolta
    printf("[Master] Raccolti tutti i %d risultati.\n", Q->num_workers);

    // --- FASE 5: Stampa Risultato ---
    // La stampa avviene qui, ma il main attenderà i join
    printf("[Master] SOMMA FINALE CALCOLATA: %lld\n", final_total_sum);

    printf("[Master] Terminato.\n");
    return NULL;
}