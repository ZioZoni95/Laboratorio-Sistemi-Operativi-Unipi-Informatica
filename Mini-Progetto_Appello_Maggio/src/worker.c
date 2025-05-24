/**
 * @file worker.c
 * @brief Implementazione della funzione eseguita dai thread Worker per l'ordinamento parallelo.
 *
 * Questa funzione orchestra le diverse fasi dell'algoritmo di ordinamento parallelo:
 * 1. (Solo Worker 0) Partizionamento dell'array e inserimento dei task (indici delle partizioni)
 * nella coda concorrente.
 * 2. (Tutti i Worker) Prelievo dei task dalla coda e ordinamento sequenziale (qsort)
 * delle partizioni assegnate.
 * 3. Sincronizzazione tramite barriera per assicurare che tutte le partizioni siano ordinate.
 * 4. (Worker attivi, in log2(P) passi) Merge parallelo delle partizioni ordinate.
 * In ogni passo k, i worker attivi uniscono coppie di blocchi, usando un array temporaneo.
 * I risultati del merge vengono poi ricopiati nell'array principale.
 * Le operazioni di merge su `temp_array` e la successiva copia su `array` sono
 * protette da mutex distinti.
 * Barriere addizionali sincronizzano i worker dopo ogni sotto-fase di merge e copia.
 * 5. Terminazione del worker.
 */

#include <math.h>   // Per log2 (o calcolo manuale di num_steps)
#include <stdlib.h> // Per qsort
#include <string.h> // Per memcpy (non usato direttamente qui per la copia principale, ma utile saperlo)
#include <assert.h> // Per assert

#include "worker.h"  // Contiene ThreadArgs, Partition_Index_Task
#include "queue.h"   // Contiene ConcurrentQueue e le sue operazioni
#include "myutils.h" // Contiene merge_sections, print_array, qsort_compare
// common.h è già incluso tramite gli altri header (worker.h o queue.h o myutils.h)

/**
 * @brief Funzione principale eseguita da ciascun thread Worker.
 * @param args Puntatore alla struttura ThreadArgs contenente i dati necessari al worker.
 * @return NULL al termine dell'esecuzione.
 */
void *worker_thread(void *args) {
    // --- 0. Setup Iniziale e Recupero Argomenti ---
    ThreadArgs *t_args = (ThreadArgs *)args; // Cast dell'argomento void* al tipo corretto
    int tid = t_args->thread_id;             // ID univoco del thread worker (0 a P-1)
    int P = t_args->n_threads;               // Numero totale di thread worker
    long N = t_args->n_elements;             // Dimensione totale dell'array da ordinare
    int *array = t_args->array;              // Puntatore all'array principale condiviso
    int *temp_array = t_args->temp_array;    // Puntatore all'array temporaneo condiviso per il merge
    ConcurrentQueue *queue = t_args->queue;  // Puntatore alla coda concorrente condivisa
    pthread_barrier_t *barrier = t_args->barrier; // Puntatore alla barriera di sincronizzazione
    pthread_mutex_t *merge_mutex = t_args->merge_mutex_ptr; // Puntatore al mutex per la serializzazione di merge_sections
    pthread_mutex_t *copy_mutex = t_args->copy_phase_mutex_ptr; // Puntatore al mutex per la serializzazione della copia
    int err; // Variabile per memorizzare i codici di ritorno delle funzioni pthread

    // Stampa di debug (o normale se DEBUG=0) indicante l'avvio del worker
    #if DEBUG == 0
    printf("[WORKER STATUS] Worker %d AVVIATO.\n", tid);
    #endif
    DEBUG_PRINT(tid, "Worker avviato. N=%ld, P=%d.", N, P);

    // --- Fase 1: Calcolo e Accodamento Partizioni (Eseguito SOLO dal Worker 0) ---
    // Solo il worker con thread_id 0 è responsabile di creare i task iniziali (partizioni).
    if (tid == 0) {
        DEBUG_PRINT(tid, "Inizio Fase 1: Calcolo e inserimento partizioni iniziali (P=%d)...", P);
        if (N > 0 && P > 0) { // Procede solo se ci sono elementi e worker
            long chunk_size = N / P;  // Dimensione base di ogni partizione
            long remainder = N % P;   // Resto della divisione, da distribuire tra le prime 'remainder' partizioni
            long current_start = 0;   // Indice di inizio della partizione corrente
            int tasks_pushed = 0;     // Contatore dei task inseriti nella coda

            // Itera per creare P partizioni
            for (int i = 0; i < P; ++i) {
                Partition_Index_Task task; // Struttura per memorizzare gli indici della partizione
                task.start = current_start;
                // Calcola la dimensione effettiva della partizione corrente, aggiungendo 1 se fa parte del resto
                long current_chunk_for_this_partition = chunk_size + (i < remainder ? 1 : 0);

                if (current_chunk_for_this_partition <= 0) { // Se la partizione calcolata è vuota o non valida
                     task.end = task.start - 1; // Imposta 'end' per renderla non valida (start > end)
                } else {
                    task.end = current_start + current_chunk_for_this_partition - 1;
                }

                // Inserisce il task nella coda solo se la partizione è valida (start <= end)
                if (task.start <= task.end) {
                    #if DEBUG == 0
                    printf("[INDICI SORTING] Worker %d (Master): Creato Task per qsort: start=%d, end=%d (elementi: %d)\n",
                            tid, task.start, task.end, task.end - task.start + 1);
                    #endif
                    DEBUG_PRINT(tid, "[Setup Fase 1] Pushing Task: start=%d, end=%d (elementi: %ld)",
                                task.start, task.end, task.end - task.start + 1);
                    push(queue, task); // Inserisce il task nella coda concorrente
                    tasks_pushed++;
                } else {
                    DEBUG_PRINT(tid, "[Setup Fase 1] Skipping Task (partizione vuota) per i=%d: start=%d, end=%d, chunk=%ld",
                                i, task.start, task.end, current_chunk_for_this_partition);
                }
                current_start += current_chunk_for_this_partition; // Aggiorna l'indice di inizio per la prossima partizione
            }
            DEBUG_PRINT(tid, "Fase 1: Inseriti %d task iniziali. Chiusura della coda...", tasks_pushed);
        } else {
             DEBUG_PRINT(tid, "Fase 1: N (%ld) o P (%d) non positivi, o N=0. Nessun task iniziale da inserire.", N, P);
        }
        close_queue(queue); // Segnala che non verranno aggiunti altri task iniziali alla coda
                            // Questo permette ai worker di terminare il pop quando la coda è vuota.
    }

    // --- Fase 2: Sorting delle Partizioni (Eseguito da TUTTI i Worker) ---
    DEBUG_PRINT(tid, "Inizio Fase 2: Sorting partizioni...");
    Partition_Index_Task current_task_qsort; // Variabile per memorizzare il task prelevato dalla coda
    int tasks_processed_by_this_thread = 0;  // Contatore dei task processati da questo specifico thread
    // Ciclo: preleva un task dalla coda finché la coda non è vuota e chiusa
    while (pop(queue, &current_task_qsort)) {
        tasks_processed_by_this_thread++;
        #if DEBUG == 0
        if (current_task_qsort.start <= current_task_qsort.end) {
            printf("[INDICI SORTING] Worker %d: Prelevato Task per qsort: start=%d, end=%d (elementi: %d)\n",
            tid, current_task_qsort.start, current_task_qsort.end, current_task_qsort.end - current_task_qsort.start + 1);
        }
        #endif
        DEBUG_PRINT(tid, "[Fase 2] Pop OK: Task(start=%d, end=%d). Eseguo qsort...",
                    current_task_qsort.start, current_task_qsort.end);
        
        // Verifica la validità degli indici del task prima di procedere con qsort
        if (current_task_qsort.start <= current_task_qsort.end &&
            current_task_qsort.start >= 0 && current_task_qsort.end < N) {
            long num_elements_in_partition = current_task_qsort.end - current_task_qsort.start + 1;
            if (num_elements_in_partition > 0) {
                 // Esegue qsort sulla porzione di array definita dal task
                 // qsort_compare è la funzione di confronto per interi
                 qsort(&array[current_task_qsort.start], num_elements_in_partition, sizeof(int), qsort_compare);
            } else {
                 DEBUG_PRINT(tid, "[Fase 2] Task(start=%d, end=%d) ha 0 elementi, qsort saltato.",
                             current_task_qsort.start, current_task_qsort.end);
            }
        } else {
            DEBUG_PRINT(tid, "[Fase 2] Task(start=%d, end=%d) non valido, vuoto o fuori range (N=%ld), qsort saltato.",
                        current_task_qsort.start, current_task_qsort.end, N);
        }
    }
    DEBUG_PRINT(tid, "Fase 2: Sorting terminato. Processati %d task da questo thread.", tasks_processed_by_this_thread);

    // --- Sincronizzazione 1: Barriera Post-Sorting ---
    // Tutti i worker attendono qui per assicurare che tutte le partizioni siano state ordinate
    // prima di procedere con la fase di merge.
    DEBUG_PRINT(tid, "Attesa su BARRIERA 1 (post-sorting)...");
    err = pthread_barrier_wait(barrier);
    // Il "serial thread" (l'ultimo ad arrivare alla barriera) ottiene PTHREAD_BARRIER_SERIAL_THREAD.
    // Questo è utile per eseguire azioni che devono avvenire una sola volta dopo la barriera.
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
        DEBUG_PRINT(tid, "Sono l'ultimo thread (serial thread) alla BARRIERA 1.");
        // Se DEBUG è attivo, stampa l'array dopo che tutte le partizioni sono state ordinate localmente.
        #if DEBUG
        if (N > 0) print_array("Array dopo Fase Sorting (Partizioni ordinate internamente)", array, N);
        else DEBUG_PRINT_GEN("Array dopo Fase Sorting: N=0, niente da stampare.");
        #endif
    } else if (err != 0) { // Qualsiasi altro valore diverso da 0 indica un errore.
        CHECK_PTHREAD_ERR(err, "Errore fatale in pthread_barrier_wait (barriera post-sort)");
    }
    DEBUG_PRINT(tid, "Superata BARRIERA 1. Inizio Fase Merge...");

    // --- Fase 3: Merge Parallelo Sincronizzato ---
    // Questa fase avviene in log2(P) passi.
    int num_steps = 0; // Numero di passi di merge necessari
    if (P > 1) { // Se c'è più di un worker, è necessario il merge
        int p_temp = P;
        while (p_temp > 1) {
            p_temp >>= 1; // Equivalente a p_temp = p_temp / 2;
            num_steps++;
         } // Calcola log_base_2(P)
    }
    DEBUG_PRINT(tid, "Numero di passi di merge necessari: %d (per P=%d)", num_steps, P);

    if (N == 0 && num_steps > 0) {
        DEBUG_PRINT(tid, "N=0. Non ci sarà alcun merge effettivo, ma si parteciperà alle barriere.");
    }

    // Ciclo per ogni passo di merge
    for (int k = 0; k < num_steps; ++k) {
        DEBUG_PRINT(tid, "--- Inizio Passo Merge k=%d ---", k);

        // Calcola il numero di worker attivi in questo passo. Si dimezza ad ogni passo.
        
        int active_workers = P >> (k + 1);
        // Determina se il worker corrente è attivo in questo passo 
        int is_active = (tid < active_workers);
        long start_index_block1 = -1, end_index_block1 = -1; // Indici del primo blocco da unire
        long start_index_block2 = -1, end_index_block2 = -1; // Indici del secondo blocco da unire
        int merge_needed_for_this_worker = 0; // Flag: 1 se questo worker deve eseguire un merge, 0 altrimenti

        // Se N > 0 solo i worker attivi  calcolano gli indici ed eseguono il merge
        if (is_active && N > 0) {
            DEBUG_PRINT(tid, "[Step %d] ATTIVO (tid=%d < active_workers=%d). Calcolo indici per il merge...", k, tid, active_workers);
            
           
           
            long base_chunk_size_original = N / P; // Dimensione base delle partizioni iniziali
            long remainder_original = N % P;       // Resto delle partizioni iniziali
            long merge_block_span_of_original_partitions = 1L << (k + 1);

            // Calcola l'indice di inizio del primo blocco (start_index_block1)
            // Sommando le dimensioni di tutte le partizioni originali che precedono
            // il gruppo di partizioni di cui questo worker è responsabile.
            start_index_block1 = 0;
            for (int i = 0; i < tid * merge_block_span_of_original_partitions; ++i) {
                start_index_block1 += base_chunk_size_original + (i < remainder_original ? 1 : 0);
            }

            // Calcola l'indice di fine del primo blocco (end_index_block1)
            long partitions_in_one_sub_block = 1L << k;
            end_index_block1 = start_index_block1 - 1; // Inizia dal precedente dell'inizio
            for (int i = 0; i < partitions_in_one_sub_block; ++i) {
                // Indice globale della partizione originale che si sta considerando
                int global_original_partition_idx = tid * merge_block_span_of_original_partitions + i;
                end_index_block1 += base_chunk_size_original + (global_original_partition_idx < remainder_original ? 1 : 0);
            }

            // L'indice di inizio del secondo blocco è immediatamente successivo alla fine del primo
            start_index_block2 = end_index_block1 + 1;

            // Controlla se il blocco 1 è valido
            if (start_index_block1 < N && end_index_block1 >= start_index_block1) {
                // Controlla se il blocco 2 inizierebbe oltre la fine dell'array
                if (start_index_block2 >= N) {
                    DEBUG_PRINT(tid,
                        "[Step %d] Blocco1(%ld-%ld) valido, ma Blocco2 inizierebbe a %ld (>=N=%ld). Nessun merge.",
                        k, start_index_block1, end_index_block1, start_index_block2, N);
                } else {
                    // Calcola l'indice di fine del secondo blocco (end_index_block2)
                    end_index_block2 = start_index_block2 - 1;
                    for (int i = 0; i < partitions_in_one_sub_block; ++i) {
                        // Indice globale della partizione originale per il secondo blocco
                        int global_original_partition_idx = tid * merge_block_span_of_original_partitions + partitions_in_one_sub_block + i;
                        end_index_block2 += base_chunk_size_original + (global_original_partition_idx < remainder_original ? 1 : 0);
                    }
                    // Assicura che end_index_block2 non superi la dimensione dell'array
                    if (end_index_block2 >= N) {
                        end_index_block2 = N - 1;
                    }

                    // Se anche il blocco 2 è valido e non vuoto, allora il merge è necessario
                    if (start_index_block2 <= end_index_block2) {
                        merge_needed_for_this_worker = 1;
                        #if DEBUG == 0
                        printf("[INDICI MERGING] Worker %d (Attivo): Passo k=%d, Unirà Blocco1 [%ld-%ld] con Blocco2 [%ld-%ld]\n",
                               tid, k, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
                        #endif
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

            // Se il merge è necessario per questo worker
            if (merge_needed_for_this_worker) {
                DEBUG_PRINT(tid,
                    "[Step %d] PRE-MERGE: Unirò array[%ld..%ld] con array[%ld..%ld] in temp_array[%ld..%ld]",
                    k, start_index_block1, end_index_block1, start_index_block2, end_index_block2,
                    start_index_block1, end_index_block2);
                
                // --- Operazione di Merge Protetta da Mutex ---
                DEBUG_PRINT(tid, "[Step %d] Tentativo di lock merge_mutex...", k);
                pthread_mutex_lock(merge_mutex); // Acquisisce il mutex per l'accesso esclusivo a temp_array
                DEBUG_PRINT(tid, "[Step %d] merge_mutex ACQUISITA. Eseguo merge_sections...", k);

                // Esegue il merge dei due blocchi dall'array principale ('array') all'array temporaneo ('temp_array')
                merge_sections(array, temp_array, start_index_block1, end_index_block1,
                               start_index_block2, end_index_block2, N);

                pthread_mutex_unlock(merge_mutex); // Rilascia il mutex
                DEBUG_PRINT(tid, "[Step %d] merge_mutex RILASCIATA.", k);
                
                // Memory barrier esplicita per assicurare che le scritture su temp_array siano visibili
                // agli altri core/thread prima di procedere
                #if defined(__GNUC__) || defined(__clang__)
                    __sync_synchronize();
                    DEBUG_PRINT(tid, "[Step %d] __sync_synchronize() chiamata dopo merge_mutex unlock.", k);
                #endif
            }
        } else if (N == 0) { // Caso speciale: array vuoto
             DEBUG_PRINT(tid, "[Step %d] Array vuoto (N=0), nessun merge.", k);
        } else { // Il worker non è attivo in questo passo di merge
             DEBUG_PRINT(tid, "[Step %d] INATTIVO (tid=%d >= active_workers=%d).", k, tid, active_workers);
        }

        // --- Sincronizzazione 2: Barriera Post-Merge-Step (prima della copia) ---
        // Tutti i worker (attivi e non) si sincronizzano qui.
        // Assicura che tutti i merge di questo passo 'k' siano completati (in temp_array)
        // prima che qualsiasi worker inizi la fase di copia.
        DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 2 (post-merge-step)...", k);
        err = pthread_barrier_wait(barrier);
        if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
             DEBUG_PRINT(tid, "[Step %d] Sono l'ultimo thread alla BARRIERA 2.", k);
        } else if (err != 0) {
             CHECK_PTHREAD_ERR(err, "Errore in pthread_barrier_wait (barriera post-merge-step)");
        }
        DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 2.", k);

        // --- Fase 3b: Copia del Risultato da temp_array ad array (Post-Barriera) ---
        // Solo i worker che hanno effettivamente eseguito un merge (merge_needed_for_this_worker == 1)
        if (merge_needed_for_this_worker) {
             long copy_s = start_index_block1; // Inizio della regione da copiare
             long copy_e = end_index_block2;   // Fine della regione da copiare (corrisponde alla fine del blocco unito)
             long num_elements_to_copy = copy_e - copy_s + 1;
             
             DEBUG_PRINT(tid,
                 "[Step %d] PRE-COPY-LOOP: Copierò temp_array[%ld...%ld] in array[%ld...%ld] (%ld elementi)",
                 k, copy_s, copy_e, copy_s, copy_e, num_elements_to_copy);
             
             // Controlli di validità degli indici di copia
             if (copy_s < 0 || copy_e >= N || copy_s > copy_e) {
                 DEBUG_PRINT(tid,
                     "[Step %d] ERRORE COPIA: Indici non validi! start=%ld, end=%ld, N=%ld",
                     k, copy_s, copy_e, N);
             } else if (num_elements_to_copy <= 0 && N > 0) { // Se non ci sono elementi da copiare (e N non è 0)
                 DEBUG_PRINT(tid,
                     "[Step %d] COPIA SALTATA: Nessun elemento da copiare. start=%ld, end=%ld, num_elem=%ld",
                     k, copy_s, copy_e, num_elements_to_copy);
             } else {
                 // --- Operazione di Copia Protetta da Mutex ---
                 DEBUG_PRINT(tid, "[Step %d] Tentativo di lock copy_mutex...", k);
                 pthread_mutex_lock(copy_mutex); // Acquisisce il mutex per la copia
                 DEBUG_PRINT(tid, "[Step %d] copy_mutex ACQUISITA. Inizio ciclo di copia.", k);
                 
                 // Copia elemento per elemento da temp_array ad array
                 for (long idx = copy_s; idx <= copy_e; ++idx) {
                     int val_read = t_args->temp_array[idx]; // Legge da temp_array (passato tramite t_args)
                     DEBUG_PRINT(tid, // Stampa di debug per ogni elemento copiato
                         "[Step %d] LOOP-COPY: Leggo temp_array[%ld]=%d. Scrivo array[%ld]=%d.",
                         k, idx, val_read, idx, val_read);
                     array[idx] = val_read; // Scrive nell'array principale
                 }
                 pthread_mutex_unlock(copy_mutex); // Rilascia il mutex
                 DEBUG_PRINT(tid, "[Step %d] copy_mutex RILASCIATA.", k);
             }
        }
        
        // --- Sincronizzazione 3: Barriera Post-Copy-Step ---
        // Tutti i worker (attivi e non) si sincronizzano nuovamente.
        // Assicura che tutte le copie da temp_array ad array per il passo 'k' siano completate
        // prima di iniziare il passo di merge successivo (k+1) o terminare la fase di merge.
        DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 3 (post-copy-step)...", k);
        err = pthread_barrier_wait(barrier);
        if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
             DEBUG_PRINT(tid, "[Step %d] Ultimo thread alla BARRIERA 3 (post-copy-step).", k);
             // Se DEBUG è attivo e P > 1, stampa l'array dopo ogni passo di merge e copia.
             #if DEBUG
             if (P > 1 && N > 0) { // Solo se c'è merge e l'array non è vuoto
                char M_label[100];
                sprintf(M_label, "Array dopo Merge Step k=%d (Copiato in array principale)", k);
                print_array(M_label, array, N);
             }
             #endif
        } else if (err != 0) {
             CHECK_PTHREAD_ERR(err, "Errore in pthread_barrier_wait (post-copy-step)");
        }
        DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 3.", k);

        DEBUG_PRINT(tid, "--- Fine Passo Merge k=%d ---", k);
    } // Fine del ciclo for sui passi di merge (k).

    DEBUG_PRINT(tid, "Fase merge completamente terminata.");

    // Stampa di debug (o normale se DEBUG=0) indicante la terminazione del worker
    #if DEBUG == 0
    printf("[WORKER STATUS] Worker %d TERMINATO.\n", tid);
    #endif
    DEBUG_PRINT(tid, "Worker in terminazione.");
    return NULL; // Termina la funzione del thread
}