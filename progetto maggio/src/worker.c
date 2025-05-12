#include <math.h>   // Per log2 (anche se calcolato manualmente)
#include <stdlib.h> // Per qsort

#include "worker.h"
#include "queue.h" // Per push, pop, close_queue
#include "myutils.h" // Per qsort_compare, merge_sections, print_array

// Funzione eseguita da ciascun thread Worker
void *worker_thread(void *args) {
    // Recupera gli argomenti specifici per questo thread
    ThreadArgs *t_args = (ThreadArgs *)args;
    int tid = t_args->thread_id;
    int P = t_args->n_threads;
    long N = t_args->n_elements;
    int *array = t_args->array;
    int *temp_array = t_args->temp_array;
    ConcurrentQueue *queue = t_args->queue;
    pthread_barrier_t *barrier = t_args->barrier;
    int err; // Variabile per controllo errori pthread

    DEBUG_PRINT(tid, "Worker avviato.");

    // ----------------------------------------------------------------------
    // Fase 0: Setup Iniziale (Eseguito solo da Worker 0)
    // Testo Esame: "All'avvio, il thread 0 (Worker 0) calcola una serie di
    //              partizioni disgiunte... e ... inserisce nella coda Q..."
    // ----------------------------------------------------------------------
    if (tid == 0) {
        DEBUG_PRINT(tid, "Calcolo e inserimento partizioni iniziali (P=%d)...", P);
        long chunk_size = N / P;  // Dimensione base
        long remainder = N % P; // Elementi extra da distribuire
        long current_start = 0; // Indice di partenza della prossima partizione
        int tasks_pushed = 0;

        // Calcola e inserisce P partizioni nella coda
        for (int i = 0; i < P; ++i) {
            Partition_Index_Task task;
            task.start = current_start;
            // Aggiunge 1 alla dimensione per le prime 'remainder' partizioni
            long current_chunk = chunk_size + (i < remainder ? 1 : 0);
            task.end = current_start + current_chunk - 1;
            if (task.end >= N) task.end = N - 1; // Sicurezza anti-sforamento

            // Inserisce il task solo se l'intervallo è valido
            if (task.start <= task.end) {
                push(queue, task);
                tasks_pushed++;
            }
            current_start += current_chunk; // Aggiorna indice di partenza
        }
        DEBUG_PRINT(tid, "Inseriti %d task iniziali. Chiusura coda...", tasks_pushed);
        // Segnala che non verranno aggiunti altri task iniziali
        close_queue(queue);
    }

    // ----------------------------------------------------------------------
    // Fase 1: Sorting Partizioni (Eseguito da tutti i Worker)
    // Testo Esame: "...tutti i Worker prelevano un task alla volta dalla coda
    //              concorrente eseguendo il sorting... (qsort()).
    //              Ogni thread ripete questa operazione finché Q non è vuota..."
    // ----------------------------------------------------------------------
    DEBUG_PRINT(tid, "Inizio fase sorting (ciclo pop/qsort)...");
    Partition_Index_Task current_task;
    int tasks_processed = 0;
    // Ciclo finché pop() restituisce 1 (task ottenuto)
    while (pop(queue, &current_task)) {
        tasks_processed++;
        DEBUG_PRINT(tid, "Pop OK: Task(%d, %d). Eseguo qsort...", current_task.start, current_task.end);
        if (current_task.start <= current_task.end) {
            qsort(&array[current_task.start],                    // Puntatore inizio sotto-array
                  current_task.end - current_task.start + 1,    // Numero elementi nel sotto-array
                  sizeof(int),                                  // Dimensione di un elemento
                  qsort_compare);                               // Funzione di confronto
        }
    }
    // pop() ha restituito 0, la coda è vuota e chiusa per questo thread
    DEBUG_PRINT(tid, "Fase sorting terminata. Processati %d task.", tasks_processed);


    // ----------------------------------------------------------------------
    // Sincronizzazione 1: Barriera Post-Sorting
    // Testo Esame: "...quindi tutti i Worker si sincronizzano mediante una BARRIERA,
    //              garantendo che ogni partizione sia stata ordinata prima di
    //              passare alla fase di merge."
    // ----------------------------------------------------------------------
    DEBUG_PRINT(tid, "Attesa su BARRIERA 1 (post-sorting)...");
    err = pthread_barrier_wait(barrier);
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
        // Questo blocco viene eseguito solo da UN thread (l'ultimo arrivato)
        DEBUG_PRINT(tid, "Sono l'ultimo thread alla BARRIERA 1.");
    } else if (err != 0) {
        // Errore nella barriera
        CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_wait (post-sort)");
    }
    // Tutti i thread riprendono l'esecuzione da qui solo dopo che P thread hanno chiamato wait
    DEBUG_PRINT(tid, "Superata BARRIERA 1 (post-sorting).");

    #if DEBUG
    // Stampa l'array dopo che tutte le partizioni iniziali sono state ordinate.
    // Solo T0 stampa per evitare output duplicato.
    if (tid == 0) {
        print_array("Array dopo Fase Sorting (Partizioni ordinate)", array, N);
    }
    #endif

    // ----------------------------------------------------------------------
    // Fase 2: Merge Sincronizzato
    // Testo Esame: "Superata la barriera, inizia la fase di merge, che viene
    //              implementata in log(base 2) P passi eseguiti in modo sincrono."
    // ----------------------------------------------------------------------
    DEBUG_PRINT(tid, "Inizio fase merge...");
    int num_steps = 0;
    if (P > 1) { // Calcola log2(P) per determinare il numero di passi
        int p_temp = P;
        while (p_temp > 1) { p_temp >>= 1; num_steps++; }
    }
    DEBUG_PRINT(tid, "Numero passi di merge necessari: %d", num_steps);

    // Ciclo sui passi di merge (k da 0 a log2(P)-1)
    for (int k = 0; k < num_steps; ++k) {
        DEBUG_PRINT(tid, "--- Inizio Passo Merge k=%d ---", k);

        // Calcola quanti worker sono attivi in questo passo e la dimensione dei blocchi
        int active_workers = P >> (k + 1); // P/2^(k+1) -> P/2, P/4, ...
        long merge_block_span = 1 << (k + 1); // Dimensione (in #partizioni iniziali) dei blocchi da unire
        int is_active = (tid < active_workers); // Questo thread è attivo se il suo ID è minore del numero di attivi

        long start_index_block1 = -1, end_index_block1 = -1;
        long start_index_block2 = -1, end_index_block2 = -1;

        // Testo Esame: "Nel passo [k+1] soltanto i primi P/2^(k+1) Worker effettuano
        //              in parallelo il merge... mentre gli altri rimangono inattivi..."
        if (is_active) {
            DEBUG_PRINT(tid, "[Step %d] ATTIVO (tid=%d < active=%d). Calcolo indici merge...", k, tid, active_workers);
            // Calcolo robusto degli indici dei due blocchi da unire
            long base_chunk_size = N / P;
            long remainder = N % P;

            // Calcola inizio del primo blocco
            start_index_block1 = 0;
            for(int i=0; i < tid * merge_block_span; ++i) {
                 start_index_block1 += base_chunk_size + (i < remainder ? 1 : 0);
            }

            // Calcola fine del primo blocco
            end_index_block1 = start_index_block1 - 1;
            long current_block_size_target = 1 << k; // #partizioni iniziali nel primo sotto-blocco
            for(int i=0; i < current_block_size_target; ++i) {
                 int global_block_idx = tid * merge_block_span + i;
                 end_index_block1 += base_chunk_size + (global_block_idx < remainder ? 1 : 0);
            }

            // Calcola inizio del secondo blocco
            start_index_block2 = end_index_block1 + 1;

            // Verifica se il secondo blocco esiste effettivamente
            if (start_index_block2 >= N) {
                 DEBUG_PRINT(tid, "[Step %d] Secondo blocco non esiste (start>=N). Salto merge.", k);
                 start_index_block2 = N; // Segnala che non c'è un secondo blocco
                 goto skip_merge_logic; // Salta il merge effettivo
            }

            // Calcola fine del secondo blocco
            end_index_block2 = end_index_block1;
            for(int i=0; i < current_block_size_target; ++i) {
                 int global_block_idx = tid * merge_block_span + current_block_size_target + i;
                 end_index_block2 += base_chunk_size + (global_block_idx < remainder ? 1 : 0);
            }
             // Assicura che l'indice finale non superi i limiti dell'array
            if (end_index_block2 >= N) end_index_block2 = N - 1;

            DEBUG_PRINT(tid, "[Step %d] Eseguo merge: Blocco1 (%ld-%ld), Blocco2 (%ld-%ld) -> temp_array",
                  k, start_index_block1, end_index_block1, start_index_block2, end_index_block2);

            // Esegue il merge dall'array sorgente all'array temporaneo
            merge_sections(array, temp_array, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
            DEBUG_PRINT(tid, "[Step %d] merge_sections completato.", k);

        } else {
            DEBUG_PRINT(tid, "[Step %d] INATTIVO.", k);
        }

    skip_merge_logic: // Etichetta per saltare il merge se non necessario

        // ------------------------------------------------------------------
        // Sincronizzazione 2: Barriera Post-Step k
        // Testo Esame: "AL termine di ogni step tutti i thread si sincronizzano
        //              nella barriera prima di iniziare lo step successivo"
        // ------------------------------------------------------------------
        DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 2 (post-step)...", k);
        err = pthread_barrier_wait(barrier);
        if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
            DEBUG_PRINT(tid, "[Step %d] Sono l'ultimo thread alla BARRIERA 2.", k);
        } else if (err != 0) {
            CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_wait (post-merge step)");
        }
        DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 2.", k);

        // Stampa lo stato dell'array DOPO la sincronizzazione di questo passo (solo T0)
        #if DEBUG
        if (tid == 0) {
             char label[100];
             snprintf(label, sizeof(label), "Array dopo Sincronizzazione Passo Merge k=%d", k);
             // Stampa array principale, che *dovrebbe* essere aggiornato dopo la memcpy sotto
             // Se volessi vedere il risultato *prima* della copia, stamperesti temp_array qui
             // (ma solo per gli intervalli modificati dai thread attivi)
             print_array(label, array, N);
        }
        #endif

        // Copia il risultato dall'array temporaneo all'array principale DOPO la barriera.
        // Questo assicura che tutti i thread attivi abbiano finito di scrivere su temp_array
        // prima che qualsiasi thread (attivo nel prossimo step) inizi a leggere da array.
        if (is_active && start_index_block2 < N) { // Copia solo se eri attivo e il merge è avvenuto
             long copy_start = start_index_block1;
             long copy_end = end_index_block2;
             long num_bytes = (copy_end - copy_start + 1) * sizeof(int);
             DEBUG_PRINT(tid, "[Step %d] Copio risultato merge da temp a array [%ld...%ld] (%ld bytes)",
                   k, copy_start, copy_end, num_bytes);
             memcpy(&array[copy_start], &temp_array[copy_start], num_bytes);
         }
         DEBUG_PRINT(tid, "--- Fine Passo Merge k=%d ---", k);
    } // Fine ciclo for sui passi di merge

    DEBUG_PRINT(tid, "Fase merge terminata.");

    // Testo Esame: "...l'intero vettore risulta ordinato ed i thread Worker terminano."
    DEBUG_PRINT(tid, "Worker terminato.");
    return NULL;
}