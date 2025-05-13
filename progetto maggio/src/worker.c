/**
 * @file worker.c
 * @brief Implementazione della funzione eseguita dai thread Worker per l'ordinamento parallelo.
 */

#include <math.h>   // Per log2 (o calcolo manuale di num_steps)
#include <stdlib.h> // Per qsort
#include <string.h> // Per memcpy (se usato, ma qui copia manuale)
#include <assert.h> // Per assert

#include "worker.h"
#include "queue.h"
#include "myutils.h" // Contiene merge_sections, print_array, ecc.
// common.h è già incluso tramite gli altri header

/**
 * @brief Funzione principale eseguita da ciascun thread Worker.
 */
void *worker_thread(void *args) {
    // --- 0. Setup Iniziale e Recupero Argomenti ---
    ThreadArgs *t_args = (ThreadArgs *)args;
    int tid = t_args->thread_id;
    int P = t_args->n_threads;
    long N = t_args->n_elements; // Dimensione totale dell'array
    // Crea puntatori locali per array e temp_array per consistenza e potenziale chiarezza.
    // Questi puntano alla stessa memoria condivisa passata tramite t_args.
    int *array = t_args->array;
    DEBUG_PRINT(tid, "Pointer t_args: %p, t_args->temp_array: %p", (void*)t_args, (void*)t_args->temp_array);

    int *temp_array = t_args->temp_array; // Usare questa variabile locale consistentemente
    ConcurrentQueue *queue = t_args->queue;
    pthread_barrier_t *barrier = t_args->barrier;
    pthread_mutex_t *merge_mutex = t_args->merge_mutex_ptr;
    int err;

    DEBUG_PRINT(tid, "Worker avviato. N=%ld, P=%d.", N, P);

    // --- Fase 1: Calcolo e Accodamento Partizioni (Eseguito SOLO dal Worker 0) ---
    if (tid == 0) {
        DEBUG_PRINT(tid, "Inizio Fase 1: Calcolo e inserimento partizioni iniziali (P=%d)...", P);
        if (N > 0 && P > 0) { // Solo se ha senso partizionare
            long chunk_size = N / P;
            long remainder = N % P;
            long current_start = 0;
            int tasks_pushed = 0;

            for (int i = 0; i < P; ++i) {
                Partition_Index_Task task;
                task.start = current_start;
                long current_chunk_for_this_partition = chunk_size + (i < remainder ? 1 : 0);

                // Se N < P, alcuni chunk potrebbero essere 0.
                // Gestisci il caso in cui il chunk calcolato sia 0 o negativo, specialmente se N è piccolo.
                if (current_chunk_for_this_partition <= 0 && N > 0) {
                     DEBUG_PRINT(tid, "[Setup Fase 1] Partizione %d ha chunk_size %ld <= 0, N=%ld. Non verrà pushato alcun task per questa partizione.", i, current_chunk_for_this_partition, N);
                     // Se N < P, le ultime P-N partizioni saranno vuote.
                     // current_start non dovrebbe avanzare se il chunk è 0.
                     // Tuttavia, la logica originale avanza current_start, che è ok
                     // se task.end < task.start implica un task vuoto/non valido gestito da qsort e merge.
                     // Per sicurezza, impostiamo end in modo che start > end
                     task.end = task.start -1; // Rende la partizione non valida
                } else if (N==0) { // Se l'array è vuoto, nessun task
                    task.end = task.start -1;
                }
                else {
                    task.end = current_start + current_chunk_for_this_partition - 1;
                }


                // Inserisce il task nella coda solo se la partizione è valida (ha almeno un elemento).
                if (task.start <= task.end) { // Controllo di validità della partizione
                    DEBUG_PRINT(tid, "[Setup Fase 1] Pushing Task: start=%d, end=%d (elementi: %ld)", task.start, task.end, task.end - task.start + 1);
                    push(queue, task);
                    tasks_pushed++;
                } else {
                    DEBUG_PRINT(tid, "[Setup Fase 1] Skipping Task (partizione vuota o non valida) per i=%d: start=%d, end=%d, chunk=%ld", i, task.start, task.end, current_chunk_for_this_partition);
                }
                current_start += current_chunk_for_this_partition;
                if (current_start > N && N > 0) { // Controllo di sanità
                    DEBUG_PRINT(tid, "[Setup Fase 1] ERRORE? current_start (%ld) > N (%ld) dopo partizione i=%d", current_start, N, i);
                    // Questo non dovrebbe accadere se la logica di partizionamento è corretta.
                }
            }
            DEBUG_PRINT(tid, "Fase 1: Inseriti %d task iniziali. Chiusura della coda...", tasks_pushed);
        } else {
             DEBUG_PRINT(tid, "Fase 1: N (%ld) o P (%d) non positivi, nessun task iniziale da inserire.", N, P);
        }
        close_queue(queue);
    }

    // --- Fase 2: Sorting delle Partizioni (Eseguito da TUTTI i Worker) ---
    DEBUG_PRINT(tid, "Inizio Fase 2: Sorting partizioni...");
    Partition_Index_Task current_task;
    int tasks_processed_by_this_thread = 0;
    while (pop(queue, &current_task)) {
        tasks_processed_by_this_thread++;
        DEBUG_PRINT(tid, "[Fase 2] Pop OK: Task(start=%d, end=%d). Eseguo qsort...", current_task.start, current_task.end);
        if (current_task.start <= current_task.end && current_task.start >=0 && current_task.end < N) { // Ulteriori controlli
            long num_elements_in_partition = current_task.end - current_task.start + 1;
            if (num_elements_in_partition > 0) { // qsort non ama 0 elementi se il puntatore non è nullo
                 qsort(&array[current_task.start], num_elements_in_partition, sizeof(int), qsort_compare);
            } else {
                 DEBUG_PRINT(tid, "[Fase 2] Task(start=%d, end=%d) ha 0 elementi, qsort saltato.", current_task.start, current_task.end);
            }
        } else {
            DEBUG_PRINT(tid, "[Fase 2] Task(start=%d, end=%d) non valido o vuoto (o fuori range N=%ld), qsort saltato.", current_task.start, current_task.end, N);
        }
    }
    DEBUG_PRINT(tid, "Fase 2: Sorting terminato. Processati %d task da questo thread.", tasks_processed_by_this_thread);

    // --- Sincronizzazione 1: Barriera Post-Sorting ---
    DEBUG_PRINT(tid, "Attesa su BARRIERA 1 (post-sorting)...");
    err = pthread_barrier_wait(barrier);
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
        DEBUG_PRINT(tid, "Sono l'ultimo thread (serial thread) alla BARRIERA 1.");
        #if DEBUG
        if (N > 0) print_array("Array dopo Fase Sorting (Partizioni ordinate internamente)", array, N);
        else DEBUG_PRINT_GEN("Array dopo Fase Sorting: N=0, niente da stampare.");
        #endif
    } else if (err != 0) {
        CHECK_PTHREAD_ERR(err, "Errore fatale in pthread_barrier_wait (barriera post-sort)");
    }
    DEBUG_PRINT(tid, "Superata BARRIERA 1. Inizio Fase Merge...");

    // --- Fase 3: Merge Parallelo Sincronizzato ---
    int num_steps = 0;
    if (P > 1) { // Solo se c'è più di un worker/partizione ha senso il merge
        int p_temp = P;
        while (p_temp > 1) { p_temp >>= 1; num_steps++; }
    }
    DEBUG_PRINT(tid, "Numero di passi di merge necessari: %d (per P=%d)", num_steps, P);

    // Se N è 0, non c'è nulla da unire.
    if (N == 0 && num_steps > 0) {
        DEBUG_PRINT(tid, "N=0, ma num_steps=%d. Non ci sarà alcun merge effettivo.", num_steps);
        // I thread parteciperanno comunque alle barriere se P > 1
    }


    for (int k = 0; k < num_steps; ++k) {
        DEBUG_PRINT(tid, "--- Inizio Passo Merge k=%d ---", k);

        int active_workers = P >> (k + 1);
        int is_active = (tid < active_workers);

        long start_index_block1 = -1, end_index_block1 = -1;
        long start_index_block2 = -1, end_index_block2 = -1;
        int merge_needed_for_this_worker = 0; // Flag per indicare se questo worker deve fare un merge

        if (is_active && N > 0) { // Solo i worker attivi e se c'è un array da processare
            DEBUG_PRINT(tid, "[Step %d] ATTIVO (tid=%d < active_workers=%d). Calcolo indici per il merge...", k, tid, active_workers);

            long base_chunk_size_original = N / P;
            long remainder_original = N % P;
            long merge_block_span_of_original_partitions = 1L << (k + 1); // Usa 1L per evitare shift su int se k è grande

            start_index_block1 = 0;
            for(int i = 0; i < tid * merge_block_span_of_original_partitions; ++i) {
                 start_index_block1 += base_chunk_size_original + (i < remainder_original ? 1 : 0);
            }

            long partitions_in_one_sub_block = 1L << k;
            end_index_block1 = start_index_block1 -1; // Inizializzazione per accumulo
            for(int i = 0; i < partitions_in_one_sub_block; ++i) {
                 int global_original_partition_idx = tid * merge_block_span_of_original_partitions + i;
                 end_index_block1 += base_chunk_size_original + (global_original_partition_idx < remainder_original ? 1 : 0);
            }

            start_index_block2 = end_index_block1 + 1;

            if (start_index_block1 < N && start_index_block2 < N && end_index_block1 >= start_index_block1) {
                // Il primo blocco è valido e c'è spazio per un potenziale secondo blocco
                end_index_block2 = start_index_block2 -1; // Inizializzazione per accumulo
                for(int i = 0; i < partitions_in_one_sub_block; ++i) {
                     int global_original_partition_idx = tid * merge_block_span_of_original_partitions + partitions_in_one_sub_block + i;
                     if ( (start_index_block2 + (end_index_block2 - (start_index_block2 -1)) ) >= N && i==0) { // Se già il primo elemento del blocco 2 è >=N
                         break; // Non aggiungere più partizioni al blocco 2 se sfora N
                     }
                     end_index_block2 += base_chunk_size_original + (global_original_partition_idx < remainder_original ? 1 : 0);
                }
                if (end_index_block2 >= N) { // Assicura che non superi la fine dell'array
                     end_index_block2 = N - 1;
                }

                // Verifica finale se il secondo blocco è valido e ha elementi
                if (start_index_block2 <= end_index_block2) { // Se il secondo blocco ha elementi
                    merge_needed_for_this_worker = 1;
                    DEBUG_PRINT(tid, "[Step %d] INDICI CALCOLATI: Blocco1(%ld-%ld) Blocco2(%ld-%ld)",
                          k, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
                } else {
                    DEBUG_PRINT(tid, "[Step %d] Blocco2 non valido o vuoto (start=%ld, end=%ld). Nessun merge per questo worker.", k, start_index_block2, end_index_block2);
                }
            } else {
                 DEBUG_PRINT(tid, "[Step %d] Blocco1 non valido (start=%ld, end=%ld) o non c'è spazio per Blocco2 (start2=%ld). Nessun merge.", k, start_index_block1, end_index_block1, start_index_block2);
            }


            if (merge_needed_for_this_worker) {
                DEBUG_PRINT(tid, "[Step %d] PRE-MERGE: Unirò array[%ld..%ld] con array[%ld..%ld] in temp_array[%ld..%ld]",
                    k, start_index_block1, end_index_block1, start_index_block2, end_index_block2,
                    start_index_block1, end_index_block2); // L'ultimo end è quello del blocco combinato

                DEBUG_PRINT(tid, "[Step %d] Tentativo di lock merge_mutex...", k);
                pthread_mutex_lock(merge_mutex);
                DEBUG_PRINT(tid, "[Step %d] merge_mutex ACQUISITA. Eseguo merge_sections...", k);

                // !!! MODIFICA: Passa N a merge_sections !!!
                merge_sections(array, temp_array, start_index_block1, end_index_block1, start_index_block2, end_index_block2, N);

                pthread_mutex_unlock(merge_mutex);
                DEBUG_PRINT(tid, "[Step %d] merge_mutex RILASCIATA.",k);

                #if DEBUG
                // Stampa i limiti del segmento appena scritto in temp_array
                if (start_index_block1 <= end_index_block2 ) { // Se c'è stato un output valido
                    char temp_label_after_merge[150];
                    snprintf(temp_label_after_merge, sizeof(temp_label_after_merge),
                             "[T%d][Step %d] temp_array NOW CONTAINS [%ld-%ld] (post-merge & unlock):",
                             tid, k, start_index_block1, end_index_block2);
                    // Stampa solo i valori limite per brevità, ma usando temp_array (locale)
                    DEBUG_PRINT(tid, "%s temp[%ld]=%d ... temp[%ld]=%d", temp_label_after_merge,
                                start_index_block1, temp_array[start_index_block1],
                                end_index_block2, temp_array[end_index_block2]);
                    fflush(stdout);
                }
                #endif
            }
        } else if (N == 0) {
             DEBUG_PRINT(tid, "[Step %d] Array vuoto (N=0), nessun merge.", k);
        }
        else { // Worker non attivo
            DEBUG_PRINT(tid, "[Step %d] INATTIVO (tid=%d >= active_workers=%d).", k, tid, active_workers);
        }

        // --- Sincronizzazione 2: Barriera Post-Step di Merge ---
        DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 2 (post-merge-step)...", k);
        err = pthread_barrier_wait(barrier);
        if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
            DEBUG_PRINT(tid, "[Step %d] Sono l'ultimo thread (serial thread) alla BARRIERA 2.", k);
            #if DEBUG
            // Il serial thread stampa temp_array completo per vedere lo stato dopo tutti i merge del passo k.
            // Usa la variabile locale temp_array, che per il serial thread è t_args->temp_array.
            if (N > 0) {
                char label_temp_array_after_all_merges[120];
                snprintf(label_temp_array_after_all_merges, sizeof(label_temp_array_after_all_merges),
                         "temp_array DOPO TUTTI I MERGE del Passo k=%d (visto da T%d, serial)", k, tid);
                print_array(label_temp_array_after_all_merges, temp_array, N);
            } else {
                DEBUG_PRINT_GEN("temp_array DOPO TUTTI I MERGE del Passo k=%d: N=0, niente da stampare.", k);
            }
            #endif
        } else if (err != 0) {
            CHECK_PTHREAD_ERR(err, "Errore fatale in pthread_barrier_wait (barriera post-merge-step)");
        }
        DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 2.", k);

        // --- Fase 3b: Copia del Risultato da temp_array ad array (Post-Barriera) ---
        // Solo i worker che erano attivi e hanno effettivamente eseguito un merge valido
        // copiano la loro porzione da 'temp_array' nuovamente in 'array'.
        if (merge_needed_for_this_worker) { // Se questo worker ha fatto un merge nel passo corrente
             long copy_s = start_index_block1;
             long copy_e = end_index_block2; // Fine del blocco combinato
             long num_elements_to_copy = copy_e - copy_s + 1;

             

             DEBUG_PRINT(tid, "[Step %d] PRE-COPY-LOOP: Copierò temp_array[%ld...%ld] in array[%ld...%ld]. (%ld elementi)",
                        k, copy_s, copy_e, copy_s, copy_e, num_elements_to_copy);

             // Controllo di validità degli indici di copia e del numero di elementi.
             if (copy_s < 0 || copy_e >= N || copy_s > copy_e ) { // Aggiunto copy_s > copy_e
                 DEBUG_PRINT(tid, "[Step %d] ERRORE COPIA: Indici non validi! start=%ld, end=%ld, N=%ld",
                       k, copy_s, copy_e, N);
             } else if (num_elements_to_copy <=0 && N > 0) {
                 DEBUG_PRINT(tid, "[Step %d] COPIA SALTATA: Nessun elemento da copiare. start=%ld, end=%ld, num_elem=%ld",
                       k, copy_s, copy_e, num_elements_to_copy);
             }
             else if (N > 0) { // Procedi solo se c'è un array
                 // Stampa di diagnostica PRIMA di leggere nel loop
                 DEBUG_PRINT(tid, "[Step %d] PRE-COPY-LOOP DBG: temp_array[%ld]=%d, temp_array[%ld]=%d (usando var locale temp_array)",
                             k, copy_s, temp_array[copy_s], copy_e, temp_array[copy_e]);
                 fflush(stdout);

                 DEBUG_PRINT(tid, "[Step %d] PRE-LOOP-COPY Check Pointers: t_args: %p, t_args->temp_array: %p, local temp_array: %p",
                        k, (void*)t_args, (void*)t_args->temp_array, (void*)temp_array);
                 fflush(stdout);


                 DEBUG_PRINT(tid, "[Step %d] DEBUG: Inizio ciclo di copia manuale da temp_array ad array.", k);
                 for (long idx = copy_s; idx <= copy_e; ++idx) {
                    if (tid == 1 && k == 0) {
    DEBUG_PRINT(tid, "[T1 k0 PRE-LOOP DETAIL] temp_array[4]=%d, temp_array[5]=%d, temp_array[6]=%d, temp_array[7]=%d",
                temp_array[4], temp_array[5], temp_array[6], temp_array[7]);
}
                     // !!! MODIFICA: Usa la variabile locale 'temp_array' consistentemente !!!
                     int val_read_from_temp = temp_array[idx];

                     DEBUG_PRINT(tid, "[Step %d] LOOP-COPY: Leggendo temp_array[%ld] (valore: %d). Scrivendo array[%ld] = %d.",
                                 k, idx, val_read_from_temp, idx, val_read_from_temp);
                     array[idx] = val_read_from_temp;
                 }
                 // !!! MODIFICA: Usa 'k' corretto nella stampa di fine loop !!!
                 DEBUG_PRINT(tid, "[Step %d] DEBUG: Fine ciclo di copia manuale.", k);
                 fflush(stdout);

                 #if DEBUG
                 // Verifica i valori appena copiati in 'array'
                 DEBUG_PRINT(tid, "[Step %d] POST-COPY-LOOP by T%d: check array[%ld]=%d, array[%ld]=%d.",
                             k, tid, copy_s, array[copy_s], copy_e, array[copy_e]);
                 fflush(stdout);
                 #endif
             }
         }
         DEBUG_PRINT(tid, "--- Fine Passo Merge k=%d ---", k);
         fflush(stdout);
    } // Fine del ciclo for sui passi di merge.

    DEBUG_PRINT(tid, "Fase merge completamente terminata.");
    DEBUG_PRINT(tid, "Worker in terminazione.");
    return NULL;
}