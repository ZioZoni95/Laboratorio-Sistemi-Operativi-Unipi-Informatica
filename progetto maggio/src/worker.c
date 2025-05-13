/**
 * @file worker.c
 * @brief Implementazione della funzione eseguita dai thread Worker per l'ordinamento parallelo.
 */

#include <math.h>   // Per log2 (o calcolo manuale di num_steps)
#include <stdlib.h> // Per qsort
#include <string.h> // Per memcpy (se usato, ma qui copia manuale)
#include <assert.h> // Per assert

#include "worker.h"  // Contiene ThreadArgs, Partition_Index_Task
#include "queue.h"   // Contiene ConcurrentQueue e le sue operazioni
#include "myutils.h" // Contiene merge_sections, print_array, qsort_compare
// common.h è già incluso tramite gli altri header (worker.h o queue.h o myutils.h)

/**
 * @brief Funzione principale eseguita da ciascun thread Worker.
 * Implementa le fasi di sorting locale delle partizioni e il merge parallelo.
 */
void *worker_thread(void *args) {
    // --- 0. Setup Iniziale e Recupero Argomenti ---
    ThreadArgs *t_args = (ThreadArgs *)args;
    int tid = t_args->thread_id;
    int P = t_args->n_threads;     // Numero totale di worker
    long N = t_args->n_elements; // Dimensione totale dell'array
    int *array = t_args->array;         // Array principale condiviso
    
    // Usa consistentemente il puntatore locale temp_array che punta alla memoria condivisa t_args->temp_array
    int *temp_array = t_args->temp_array; 

    ConcurrentQueue *queue = t_args->queue;
    pthread_barrier_t *barrier = t_args->barrier;
    pthread_mutex_t *merge_mutex = t_args->merge_mutex_ptr;     // Mutex per serializzare le chiamate a merge_sections
    pthread_mutex_t *copy_mutex = t_args->copy_phase_mutex_ptr; // Mutex per serializzare la fase di copia temp->array
    int err; // Per codici di errore pthread

    DEBUG_PRINT(tid, "Worker avviato. N=%ld, P=%d.", N, P);
    DEBUG_PRINT(tid, "Pointer t_args: %p, t_args->array: %p, t_args->temp_array: %p", 
                (void*)t_args, (void*)array, (void*)temp_array);


    // --- Fase 1: Calcolo e Accodamento Partizioni (Eseguito SOLO dal Worker 0) ---
    if (tid == 0) {
        DEBUG_PRINT(tid, "Inizio Fase 1: Calcolo e inserimento partizioni iniziali (P=%d)...", P);
        if (N > 0 && P > 0) {
            long chunk_size = N / P;
            long remainder = N % P;
            long current_start = 0;
            int tasks_pushed = 0;

            for (int i = 0; i < P; ++i) {
                Partition_Index_Task task;
                task.start = current_start;
                long current_chunk_for_this_partition = chunk_size + (i < remainder ? 1 : 0);

                if (current_chunk_for_this_partition <= 0) { // Partizione vuota o non valida
                     task.end = task.start - 1; 
                } else {
                    task.end = current_start + current_chunk_for_this_partition - 1;
                }

                if (task.start <= task.end) {
                    DEBUG_PRINT(tid, "[Setup Fase 1] Pushing Task: start=%d, end=%d (elementi: %ld)", task.start, task.end, task.end - task.start + 1);
                    push(queue, task);
                    tasks_pushed++;
                } else {
                    DEBUG_PRINT(tid, "[Setup Fase 1] Skipping Task (partizione vuota) per i=%d: start=%d, end=%d, chunk=%ld", i, task.start, task.end, current_chunk_for_this_partition);
                }
                current_start += current_chunk_for_this_partition;
            }
            DEBUG_PRINT(tid, "Fase 1: Inseriti %d task iniziali. Chiusura della coda...", tasks_pushed);
        } else {
             DEBUG_PRINT(tid, "Fase 1: N (%ld) o P (%d) non positivi, o N=0. Nessun task iniziale da inserire.", N, P);
        }
        close_queue(queue); // Segnala che non verranno aggiunti altri task iniziali
    }

    // --- Fase 2: Sorting delle Partizioni (Eseguito da TUTTI i Worker) ---
    DEBUG_PRINT(tid, "Inizio Fase 2: Sorting partizioni...");
    Partition_Index_Task current_task_qsort;
    int tasks_processed_by_this_thread = 0;
    while (pop(queue, &current_task_qsort)) { // Preleva un task dalla coda
        tasks_processed_by_this_thread++;
        DEBUG_PRINT(tid, "[Fase 2] Pop OK: Task(start=%d, end=%d). Eseguo qsort...", current_task_qsort.start, current_task_qsort.end);
        
        // Verifica validità indici e numero elementi
        if (current_task_qsort.start <= current_task_qsort.end && 
            current_task_qsort.start >= 0 && current_task_qsort.end < N) {
            long num_elements_in_partition = current_task_qsort.end - current_task_qsort.start + 1;
            if (num_elements_in_partition > 0) {
                 qsort(&array[current_task_qsort.start], num_elements_in_partition, sizeof(int), qsort_compare);
            } else {
                 DEBUG_PRINT(tid, "[Fase 2] Task(start=%d, end=%d) ha 0 elementi, qsort saltato.", current_task_qsort.start, current_task_qsort.end);
            }
        } else {
            DEBUG_PRINT(tid, "[Fase 2] Task(start=%d, end=%d) non valido, vuoto o fuori range (N=%ld), qsort saltato.", current_task_qsort.start, current_task_qsort.end, N);
        }
    }
    DEBUG_PRINT(tid, "Fase 2: Sorting terminato. Processati %d task da questo thread.", tasks_processed_by_this_thread);

    // --- Sincronizzazione 1: Barriera Post-Sorting ---
    // Tutti i worker attendono qui per assicurare che tutte le partizioni siano ordinate.
    DEBUG_PRINT(tid, "Attesa su BARRIERA 1 (post-sorting)...");
    err = pthread_barrier_wait(barrier);
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) { // Solo un thread (il "serial thread") ritorna questo valore
        DEBUG_PRINT(tid, "Sono l'ultimo thread (serial thread) alla BARRIERA 1.");
        #if DEBUG
        if (N > 0) print_array("Array dopo Fase Sorting (Partizioni ordinate internamente)", array, N);
        else DEBUG_PRINT_GEN("Array dopo Fase Sorting: N=0, niente da stampare.");
        #endif
    } else if (err != 0) { // Qualsiasi altro valore diverso da 0 e PTHREAD_BARRIER_SERIAL_THREAD è un errore
        CHECK_PTHREAD_ERR(err, "Errore fatale in pthread_barrier_wait (barriera post-sort)");
    }
    DEBUG_PRINT(tid, "Superata BARRIERA 1. Inizio Fase Merge...");

    // --- Fase 3: Merge Parallelo Sincronizzato ---
    int num_steps = 0;
    if (P > 1) {
        int p_temp = P;
        while (p_temp > 1) { 
            p_temp >>= 1; /*shift a destra per dividere per 2*/ 
            num_steps++;
         } // Calcola log2(P)
    }
    DEBUG_PRINT(tid, "Numero di passi di merge necessari: %d (per P=%d)", num_steps, P);

    if (N == 0 && num_steps > 0) {
        DEBUG_PRINT(tid, "N=0. Non ci sarà alcun merge effettivo, ma si parteciperà alle barriere.");
    }

for (int k = 0; k < num_steps; ++k) {
    DEBUG_PRINT(tid, "--- Inizio Passo Merge k=%d ---", k);

    int active_workers = P >> (k + 1); // Numero di worker attivi in questo passo
    int is_active = (tid < active_workers);
    long start_index_block1 = -1, end_index_block1 = -1;
    long start_index_block2 = -1, end_index_block2 = -1;
    int merge_needed_for_this_worker = 0;

    if (is_active && N > 0) {
        DEBUG_PRINT(tid, "[Step %d] ATTIVO (tid=%d < active_workers=%d). Calcolo indici per il merge...", k, tid, active_workers);
        
        long base_chunk_size_original = N / P;
        long remainder_original = N % P;
        long merge_block_span_of_original_partitions = 1L << (k + 1);

        // Calcola start_index_block1 come somma delle dimensioni delle partizioni precedenti
        start_index_block1 = 0;
        for (int i = 0; i < tid * merge_block_span_of_original_partitions; ++i) {
            start_index_block1 += base_chunk_size_original + (i < remainder_original ? 1 : 0);
        }

        long partitions_in_one_sub_block = 1L << k;
        end_index_block1 = start_index_block1 - 1;
        for (int i = 0; i < partitions_in_one_sub_block; ++i) {
            int global_original_partition_idx = tid * merge_block_span_of_original_partitions + i;
            end_index_block1 += base_chunk_size_original + (global_original_partition_idx < remainder_original ? 1 : 0);
        }

        start_index_block2 = end_index_block1 + 1;

        if (start_index_block1 < N && end_index_block1 >= start_index_block1) {
            if (start_index_block2 >= N) {
                DEBUG_PRINT(tid,
                    "[Step %d] Blocco1(%ld-%ld) valido, ma Blocco2 inizierebbe a %ld (>=N=%ld). Nessun merge.",
                    k, start_index_block1, end_index_block1, start_index_block2, N);
            } else {
                end_index_block2 = start_index_block2 - 1;
                for (int i = 0; i < partitions_in_one_sub_block; ++i) {
                    int global_original_partition_idx = tid * merge_block_span_of_original_partitions + partitions_in_one_sub_block + i;
                    end_index_block2 += base_chunk_size_original + (global_original_partition_idx < remainder_original ? 1 : 0);
                }
                if (end_index_block2 >= N) {
                    end_index_block2 = N - 1;
                }
                if (start_index_block2 <= end_index_block2) {
                    merge_needed_for_this_worker = 1;
                    DEBUG_PRINT(tid,
                        "[Step %d] INDICI CALCOLATI: Blocco1(%ld-%ld) Blocco2(%ld-%ld)",
                        k, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
                } else {
                    DEBUG_PRINT(tid,
                        "[Step %d] Blocco2 non valido o vuoto (start=%ld, end=%ld). Nessun merge per questo worker.",
                        k, start_index_block2, end_index_block2);
                }
            }
        } else {
            DEBUG_PRINT(tid,
                "[Step %d] Blocco1 non valido (start=%ld, end=%ld). Nessun merge.",
                k, start_index_block1, end_index_block1);
        }

        if (merge_needed_for_this_worker) {
            DEBUG_PRINT(tid,
                "[Step %d] PRE-MERGE: Unirò array[%ld..%ld] con array[%ld..%ld] in temp_array[%ld..%ld]",
                k, start_index_block1, end_index_block1, start_index_block2, end_index_block2,
                start_index_block1, end_index_block2);
            
            DEBUG_PRINT(tid, "[Step %d] Tentativo di lock merge_mutex...", k);
            pthread_mutex_lock(merge_mutex);
            DEBUG_PRINT(tid, "[Step %d] merge_mutex ACQUISITA. Eseguo merge_sections...", k);

            merge_sections(array, temp_array, start_index_block1, end_index_block1,
                           start_index_block2, end_index_block2, N);

            pthread_mutex_unlock(merge_mutex);
            DEBUG_PRINT(tid, "[Step %d] merge_mutex RILASCIATA.", k);
            
            #if defined(__GNUC__) || defined(__clang__)
                __sync_synchronize();
                DEBUG_PRINT(tid, "[Step %d] __sync_synchronize() chiamata dopo merge_mutex unlock.", k);
            #endif
        }
    } else if (N == 0) {
         DEBUG_PRINT(tid, "[Step %d] Array vuoto (N=0), nessun merge.", k);
    } else {
         DEBUG_PRINT(tid, "[Step %d] INATTIVO (tid=%d >= active_workers=%d).", k, tid, active_workers);
    }

    /* --- Barriera post-merge (prima della copia) --- */
    DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 2 (post-merge-step)...", k);
    err = pthread_barrier_wait(barrier);
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
         DEBUG_PRINT(tid, "[Step %d] Sono l'ultimo thread alla BARRIERA 2.", k);
    } else if (err != 0) {
         CHECK_PTHREAD_ERR(err, "Errore in pthread_barrier_wait (barriera post-merge-step)");
    }
    DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 2.", k);

    /* --- Fase 3b: Copia del Risultato da temp_array ad array (Post-Barriera) --- */
    if (merge_needed_for_this_worker) {
         long copy_s = start_index_block1;
         long copy_e = end_index_block2;
         long num_elements_to_copy = copy_e - copy_s + 1;
         
         DEBUG_PRINT(tid,
             "[Step %d] PRE-COPY-LOOP: Copierò temp_array[%ld...%ld] in array[%ld...%ld] (%ld elementi)",
             k, copy_s, copy_e, copy_s, copy_e, num_elements_to_copy);
         
         if (copy_s < 0 || copy_e >= N || copy_s > copy_e) {
             DEBUG_PRINT(tid,
                 "[Step %d] ERRORE COPIA: Indici non validi! start=%ld, end=%ld, N=%ld",
                 k, copy_s, copy_e, N);
         } else if (num_elements_to_copy <= 0 && N > 0) {
             DEBUG_PRINT(tid,
                 "[Step %d] COPIA SALTATA: Nessun elemento da copiare. start=%ld, end=%ld, num_elem=%ld",
                 k, copy_s, copy_e, num_elements_to_copy);
         } else {
             DEBUG_PRINT(tid, "[Step %d] Tentativo di lock copy_mutex...", k);
             pthread_mutex_lock(copy_mutex);
             DEBUG_PRINT(tid, "[Step %d] copy_mutex ACQUISITA. Inizio ciclo di copia.", k);
             
             for (long idx = copy_s; idx <= copy_e; ++idx) {
                 int val_read = t_args->temp_array[idx];
                 DEBUG_PRINT(tid,
                     "[Step %d] LOOP-COPY: Leggo temp_array[%ld]=%d. Scrivo array[%ld]=%d.",
                     k, idx, val_read, idx, val_read);
                 array[idx] = val_read;
             }
             pthread_mutex_unlock(copy_mutex);
             DEBUG_PRINT(tid, "[Step %d] copy_mutex RILASCIATA.", k);
         }
    }
    
    /* --- Nuova barriera post-copy per sincronizzare TUTTI i thread --- */
    DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 3 (post-copy-step)...", k);
    err = pthread_barrier_wait(barrier);
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
         DEBUG_PRINT(tid, "[Step %d] Ultimo thread alla BARRIERA 3 (post-copy-step).", k);
    } else if (err != 0) {
         CHECK_PTHREAD_ERR(err, "Errore in pthread_barrier_wait (post-copy-step)");
    }
    DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 3.", k);

    DEBUG_PRINT(tid, "--- Fine Passo Merge k=%d ---", k);
} // Fine del ciclo for sui passi di merge.

    DEBUG_PRINT(tid, "Fase merge completamente terminata.");
    DEBUG_PRINT(tid, "Worker in terminazione.");
    return NULL;
}