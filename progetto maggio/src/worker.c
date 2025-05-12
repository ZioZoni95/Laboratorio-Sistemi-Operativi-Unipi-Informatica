/**
 * @file worker.c
 * @brief Implementazione della funzione eseguita dai thread Worker per l'ordinamento parallelo.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h> // Per memcpy (anche se attualmente sostituito da loop manuale per debug)

#include "worker.h"
#include "queue.h"
#include "myutils.h"

/**
 * @brief Funzione principale eseguita da ciascun thread Worker.
 */
void *worker_thread(void *args) {
    // --- 0. Setup Iniziale e Recupero Argomenti ---
    ThreadArgs *t_args = (ThreadArgs *)args;
    int tid = t_args->thread_id;
    int P = t_args->n_threads;
    long N = t_args->n_elements;
    int *array = t_args->array;
    int *temp_array = t_args->temp_array;
    ConcurrentQueue *queue = t_args->queue;
    pthread_barrier_t *barrier = t_args->barrier;
    pthread_mutex_t *merge_mutex = t_args->merge_mutex_ptr; // Recupera il puntatore alla mutex
    int err;

    DEBUG_PRINT(tid, "Worker avviato.");

    // --- Fase 1: Calcolo e Accodamento Partizioni (Solo Worker 0) ---
    if (tid == 0) {
        DEBUG_PRINT(tid, "Calcolo e inserimento partizioni iniziali (P=%d)...", P);
        long chunk_size = N / P;
        long remainder = N % P;
        long current_start = 0;
        int tasks_pushed = 0;

        for (int i = 0; i < P; ++i) {
            Partition_Index_Task task;
            task.start = current_start;
            long current_chunk = chunk_size + (i < remainder ? 1 : 0);
            task.end = current_start + current_chunk - 1;

            if (task.start <= task.end && current_chunk > 0) {
                DEBUG_PRINT(tid, "[Setup] Pushing Task: start=%d, end=%d (chunk=%ld)", task.start, task.end, current_chunk);
                push(queue, task);
                tasks_pushed++;
            } else {
                 DEBUG_PRINT(tid, "[Setup] Skipping Task for partition %d (chunk=%ld, start=%ld, end=%ld)", i, current_chunk, task.start, task.end);
            }
            current_start += current_chunk;
        }
        DEBUG_PRINT(tid, "Inseriti %d task iniziali. Chiusura coda...", tasks_pushed);
        close_queue(queue);
    }

    // --- Fase 2: Sorting Partizioni (Tutti i Worker) ---
    DEBUG_PRINT(tid, "Inizio fase sorting (ciclo pop/qsort)...");
    Partition_Index_Task current_task;
    int tasks_processed_by_this_thread = 0;
    while (pop(queue, &current_task)) {
        tasks_processed_by_this_thread++;
        DEBUG_PRINT(tid, "Pop OK: Task(start=%d, end=%d). Eseguo qsort...", current_task.start, current_task.end);
        if (current_task.start <= current_task.end) {
            long num_elements_in_partition = current_task.end - current_task.start + 1;
            qsort(&array[current_task.start], num_elements_in_partition, sizeof(int), qsort_compare);
        } else {
            DEBUG_PRINT(tid, "Task(start=%d, end=%d) non valido o vuoto, qsort saltato.", current_task.start, current_task.end);
        }
    }
    DEBUG_PRINT(tid, "Fase sorting terminata. Processati %d task da questo thread.", tasks_processed_by_this_thread);

    // --- Sincronizzazione 1: Barriera Post-Sorting ---
    DEBUG_PRINT(tid, "Attesa su BARRIERA 1 (post-sorting)...");
    err = pthread_barrier_wait(barrier);
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
        DEBUG_PRINT(tid, "Sono l'ultimo thread alla BARRIERA 1.");
        #if DEBUG
        print_array("Array dopo Fase Sorting (Partizioni ordinate)", array, N);
        #endif
    } else if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
        CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_wait (post-sort)");
    }
    DEBUG_PRINT(tid, "Superata BARRIERA 1 (post-sorting).");

    // --- Fase 3: Merge Parallelo Sincronizzato ---
    DEBUG_PRINT(tid, "Inizio fase merge...");
    int num_steps = 0;
    if (P > 1) {
        int p_temp = P;
        while (p_temp > 1) { p_temp >>= 1; num_steps++; }
    }
    DEBUG_PRINT(tid, "Numero passi di merge necessari: %d", num_steps);

    for (int k = 0; k < num_steps; ++k) {
        DEBUG_PRINT(tid, "--- Inizio Passo Merge k=%d ---", k);
        int active_workers = P >> (k + 1);
        int is_active = (tid < active_workers);
        long start_index_block1 = -1, end_index_block1 = -1;
        long start_index_block2 = -1, end_index_block2 = -1;

        if (is_active) {
            DEBUG_PRINT(tid, "[Step %d] ATTIVO (tid=%d < active=%d). Calcolo indici merge...", k, tid, active_workers);
            long base_chunk_size = N / P;
            long remainder = N % P;
            long merge_block_span = 1 << (k + 1);
            start_index_block1 = 0;
            for(int i = 0; i < tid * merge_block_span; ++i) {
                 start_index_block1 += base_chunk_size + (i < remainder ? 1 : 0);
            }
            end_index_block1 = start_index_block1 - 1;
            long partitions_in_sub_block = 1 << k;
            for(int i = 0; i < partitions_in_sub_block; ++i) {
                 int global_block_idx = tid * merge_block_span + i;
                 end_index_block1 += base_chunk_size + (global_block_idx < remainder ? 1 : 0);
            }
            start_index_block2 = end_index_block1 + 1;

            if (start_index_block2 >= N) {
                 DEBUG_PRINT(tid, "[Step %d] Secondo blocco non esiste (start2=%ld >= N=%ld). Salto merge e copia.", k, start_index_block2, N);
                 start_index_block2 = N;
                 goto skip_merge_logic;
            }
            end_index_block2 = end_index_block1;
            for(int i = 0; i < partitions_in_sub_block; ++i) {
                 int global_block_idx = tid * merge_block_span + partitions_in_sub_block + i;
                 end_index_block2 += base_chunk_size + (global_block_idx < remainder ? 1 : 0);
            }
            if (end_index_block2 >= N) {
                 end_index_block2 = N - 1;
            }

            DEBUG_PRINT(tid, "[Step %d] INDICI CALCOLATI: Blocco1(%ld-%ld) Blocco2(%ld-%ld)",
                  k, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
            if (end_index_block1 < start_index_block1 && N > 0) {
                 DEBUG_PRINT(tid, "[Step %d] ATTENZIONE: end_index_block1 (%ld) < start_index_block1 (%ld)!", k, end_index_block1, start_index_block1);
            }
             if (end_index_block2 < start_index_block2 && start_index_block2 < N) {
                 DEBUG_PRINT(tid, "[Step %d] ATTENZIONE: end_index_block2 (%ld) < start_index_block2 (%ld)!", k, end_index_block2, start_index_block2);
            }
            DEBUG_PRINT(tid, "[Step %d] PRE-MERGE Check: Target indices in temp_array: [%ld...%ld]", k, start_index_block1, end_index_block2);

            // --- INIZIO SEZIONE CRITICA PER temp_array ---
            // Blocca la mutex prima di chiamare merge_sections, che scrive su temp_array.
            // Questo serializza le operazioni di merge per prevenire race conditions su temp_array.
            DEBUG_PRINT(tid, "[Step %d] Tentativo di lock merge_mutex...", k);
            pthread_mutex_lock(merge_mutex);
            DEBUG_PRINT(tid, "[Step %d] merge_mutex ACQUISITA. Eseguo merge...", k);

            merge_sections(array, temp_array, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
            
            // Sblocca la mutex dopo che merge_sections ha completato la scrittura su temp_array.
            pthread_mutex_unlock(merge_mutex);
// --- AGGIUNGI QUESTA STAMPA SUBITO DOPO UNLOCK E PRIMA DELLA BARRIERA ---
            #if DEBUG
            if (is_active && start_index_block2 < N) { // Stampa solo se hai fatto merge
                char temp_label[150];
                snprintf(temp_label, sizeof(temp_label),
                         "[T%d][Step %d] Contenuto temp_array [%ld-%ld] DOPO merge & unlock mutex:",
                         tid, k, start_index_block1, end_index_block2);
                // Stampa solo la porzione di temp_array modificata da questo thread
                // Per fare ciò correttamente, avremmo bisogno di una funzione print_partial_array
                // Per ora, stampiamo i primi e gli ultimi elementi del blocco modificato.
                if (end_index_block2 >= start_index_block1) { // se il blocco è valido
                     DEBUG_PRINT(tid, "%s temp[%ld]=%d ... temp[%ld]=%d", temp_label,
                                 start_index_block1, temp_array[start_index_block1],
                                 end_index_block2, temp_array[end_index_block2]);
                }
            }
            #endif
            // --- FINE STAMPA AGGIUNTA --- 
                       // --- FINE SEZIONE CRITICA PER temp_array ---

        } else {
            DEBUG_PRINT(tid, "[Step %d] INATTIVO.", k);
        }

    skip_merge_logic:
        DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 2 (post-step)...", k);
        err = pthread_barrier_wait(barrier);
        if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
            DEBUG_PRINT(tid, "[Step %d] Sono l'ultimo thread alla BARRIERA 2.", k);
            #if DEBUG
            char label[120]; // Aumentata dimensione per nome più lungo
            // Stampa l'array DOPO la sincronizzazione ma PRIMA della copia da temp ad array
            // da parte del thread corrente. Potrebbe già riflettere copie di altri thread.
            snprintf(label, sizeof(label), "Array dopo Sincronizzazione Passo Merge k=%d (Prima della Copia da T%d)", k, tid);
            print_array(label, array, N);
            #endif
        } else if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
            CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_wait (post-merge step)");
        }
        DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 2.", k);

        // Fase 3b: Copia Risultato da temp_array ad array (Post-Barriera)
        if (is_active && start_index_block2 < N) {
             long copy_start = start_index_block1;
             long copy_end = end_index_block2;
             long num_elements_to_copy = copy_end - copy_start + 1;

             DEBUG_PRINT(tid, "[Step %d] COPIO RISULTATO: da temp[%ld...%ld] a array[%ld...%ld] (%ld elementi)",
                   k, copy_start, copy_end, copy_start, copy_end, num_elements_to_copy);

             if (copy_start < 0 || copy_end >= N || num_elements_to_copy <= 0) {
                 DEBUG_PRINT(tid, "[Step %d] ERRORE COPIA MANUALE: Indici non validi! start=%ld, end=%ld, N=%ld",
                       k, copy_start, copy_end, N);
             } else {
                 DEBUG_PRINT(tid, "[Step %d] DEBUG: Inizio copia manuale. temp[%ld]=%d, temp[%ld]=%d", k, copy_start, temp_array[copy_start], copy_end, temp_array[copy_end]);
                 for (long idx = copy_start; idx <= copy_end; ++idx) {
                     if (idx >= N) {
                        DEBUG_PRINT(tid, "[Step %d] ERRORE BOUNDS: idx=%ld >= N=%ld durante copia manuale!", k, idx, N);
                        break;
                     }
                     array[idx] = temp_array[idx];
                 }
                 DEBUG_PRINT(tid, "[Step %d] DEBUG: Fine copia manuale. array[%ld]=%d, array[%ld]=%d", k, copy_start, array[copy_start], copy_end, array[copy_end]);
             }
         }
         DEBUG_PRINT(tid, "--- Fine Passo Merge k=%d ---", k);
    }

    DEBUG_PRINT(tid, "Fase merge terminata.");
    DEBUG_PRINT(tid, "Worker terminato.");
    return NULL;
}