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
 * Implementa l'algoritmo di ordinamento parallelo come descritto nella traccia d'esame.
 */
void *worker_thread(void *args) {
    // --- 0. Setup Iniziale e Recupero Argomenti ---
    // Converte l'argomento void* al tipo atteso ThreadArgs.
    ThreadArgs *t_args = (ThreadArgs *)args;
    // Estrae le variabili dalla struttura ThreadArgs per un accesso più semplice.
    int tid = t_args->thread_id;            // ID univoco di questo thread (da 0 a P-1).
    int P = t_args->n_threads;              // Numero totale di thread Worker.
    long N = t_args->n_elements;            // Numero totale di elementi nell'array.
    int *array = t_args->array;             // Puntatore all'array condiviso da ordinare.
    int *temp_array = t_args->temp_array;   // Puntatore all'array temporaneo usato nella fase di merge.
    ConcurrentQueue *queue = t_args->queue; // Puntatore alla coda concorrente condivisa.
    pthread_barrier_t *barrier = t_args->barrier; // Puntatore alla barriera di sincronizzazione.
    pthread_mutex_t *merge_mutex = t_args->merge_mutex_ptr; // Puntatore alla mutex per la serializzazione dei merge.
    int err; // Variabile per memorizzare i codici di errore delle funzioni pthread.

    DEBUG_PRINT(tid, "Worker avviato.");

    // --- Fase 1: Calcolo e Accodamento Partizioni (Eseguito SOLO dal Worker 0) ---
    if (tid == 0) {
        DEBUG_PRINT(tid, "Inizio Fase 1: Calcolo e inserimento partizioni iniziali (P=%d)...", P);
        long chunk_size = N / P; // Dimensione base di ogni partizione.
        long remainder = N % P;  // Resto della divisione N/P, da distribuire tra le prime 'remainder' partizioni.
        long current_start = 0;  // Indice di inizio della partizione corrente.
        int tasks_pushed = 0;    // Contatore per i task inseriti nella coda.

        // Itera P volte per definire P partizioni.
        for (int i = 0; i < P; ++i) {
            Partition_Index_Task task;
            task.start = current_start;
            // Calcola la dimensione effettiva della partizione corrente, aggiungendo 1 se fa parte del 'remainder'.
            long current_chunk_for_this_partition = chunk_size + (i < remainder ? 1 : 0);
            task.end = current_start + current_chunk_for_this_partition - 1;

            // Inserisce il task nella coda solo se la partizione è valida (ha almeno un elemento).
            if (task.start <= task.end && current_chunk_for_this_partition > 0) {
                DEBUG_PRINT(tid, "[Setup Fase 1] Pushing Task: start=%d, end=%d (elementi: %ld)", task.start, task.end, current_chunk_for_this_partition);
                push(queue, task); // Inserisce il task (coppia start, end) nella coda.
                tasks_pushed++;
            } else {
                 DEBUG_PRINT(tid, "[Setup Fase 1] Skipping Task (partizione vuota o non valida) per i=%d: start=%ld, end=%ld, chunk=%ld", i, task.start, task.end, current_chunk_for_this_partition);
            }
            current_start += current_chunk_for_this_partition; // Aggiorna l'indice di inizio per la prossima partizione.
        }
        DEBUG_PRINT(tid, "Fase 1: Inseriti %d task iniziali. Chiusura della coda (nessun altro task iniziale verrà aggiunto)...", tasks_pushed);
        close_queue(queue); // Segnala che non verranno aggiunti altri task iniziali.
    }

    // --- Fase 2: Sorting delle Partizioni (Eseguito da TUTTI i Worker) ---
    // Ogni worker preleva task dalla coda e ordina la partizione corrispondente.
    DEBUG_PRINT(tid, "Inizio Fase 2: Sorting partizioni (ciclo pop/qsort)...");
    Partition_Index_Task current_task;
    int tasks_processed_by_this_thread = 0; // Contatore locale per i task processati da questo thread.
    // Cicla finché 'pop' restituisce 1 (un task è stato prelevato).
    // 'pop' restituisce 0 se la coda è vuota E chiusa.
    while (pop(queue, &current_task)) {
        tasks_processed_by_this_thread++;
        DEBUG_PRINT(tid, "[Fase 2] Pop OK: Task(start=%d, end=%d). Eseguo qsort...", current_task.start, current_task.end);
        // Verifica che il task sia valido (contenga almeno un elemento).
        if (current_task.start <= current_task.end) {
            long num_elements_in_partition = current_task.end - current_task.start + 1;
            // Usa qsort (libreria standard C) per ordinare la porzione di array specificata dal task.
            qsort(&array[current_task.start], num_elements_in_partition, sizeof(int), qsort_compare);
        } else {
            DEBUG_PRINT(tid, "[Fase 2] Task(start=%d, end=%d) non valido o vuoto, qsort saltato.", current_task.start, current_task.end);
        }
    }
    DEBUG_PRINT(tid, "Fase 2: Sorting terminato. Processati %d task da questo thread.", tasks_processed_by_this_thread);

    // --- Sincronizzazione 1: Barriera Post-Sorting ---
    // Tutti i worker attendono qui finché ogni partizione non è stata ordinata.
    // Questo garantisce che la fase di merge inizi solo dopo il completamento di tutti i sort.
    DEBUG_PRINT(tid, "Attesa su BARRIERA 1 (post-sorting)...");
    err = pthread_barrier_wait(barrier);
    // pthread_barrier_wait restituisce PTHREAD_BARRIER_SERIAL_THREAD a UN SOLO thread (il "serial thread").
    // Tutti gli altri thread ricevono 0 in caso di successo.
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
        DEBUG_PRINT(tid, "Sono l'ultimo thread (serial thread) alla BARRIERA 1.");
        #if DEBUG // Stampa l'array solo se DEBUG è abilitato.
        // A questo punto, ogni partizione è ordinata individualmente, ma l'array completo non lo è ancora.
        print_array("Array dopo Fase Sorting (Partizioni ordinate internamente)", array, N);
        fflush(stdout); // Assicura che la stampa sia visibile prima che altri thread procedano.
        #endif
    } else if (err != 0) { // Controlla se pthread_barrier_wait ha restituito un errore.
        CHECK_PTHREAD_ERR(err, "Errore fatale in pthread_barrier_wait (barriera post-sort)");
    }
    DEBUG_PRINT(tid, "Superata BARRIERA 1 (post-sorting). Inizio Fase Merge...");

    // --- Fase 3: Merge Parallelo Sincronizzato ---
    // Questa fase è implementata in log2(P) passi.
    // In ogni passo 'k', il numero di worker attivi si dimezza e fondono blocchi di dimensione crescente.
    
    int num_steps = 0; // Numero di passi di merge necessari.
    if (P > 1) { // Se P=1, non c'è merge da fare.
        // Calcola log2(P) per determinare il numero di passi.
        int p_temp = P;
        while (p_temp > 1) { p_temp >>= 1; num_steps++; } // Equivale a floor(log2(P)).
    }
    DEBUG_PRINT(tid, "Numero di passi di merge necessari: %d (per P=%d)", num_steps, P);

    for (int k = 0; k < num_steps; ++k) { // Itera per ogni passo di merge.
        DEBUG_PRINT(tid, "--- Inizio Passo Merge k=%d ---", k);

        // Determina quanti worker sono attivi in questo passo 'k'.
        // Passo 0: P/2 attivi. Passo 1: P/4 attivi. E così via.
        int active_workers = P >> (k + 1); 
        // Verifica se questo thread (tid) è uno dei worker attivi.
        int is_active = (tid < active_workers); 

        // Variabili per gli indici dei due blocchi da unire.
        // Inizializzate a valori non validi per sicurezza.
        long start_index_block1 = -1, end_index_block1 = -1;
        long start_index_block2 = -1, end_index_block2 = -1;

        if (is_active) {
            DEBUG_PRINT(tid, "[Step %d] ATTIVO (tid=%d < active_workers=%d). Calcolo indici per il merge...", k, tid, active_workers);
            
            // Logica per calcolare gli indici dei due blocchi che questo worker deve unire.
            // Questa logica deve tenere conto della dimensione originale delle partizioni (incl. il remainder).
            long base_chunk_size_original = N / P;
            long remainder_original = N % P;
            
            // 'merge_block_span_of_original_partitions' indica quante partizioni *originali*
            // sono coperte complessivamente dai due blocchi che questo worker attivo andrà a gestire.
            // Esempio: k=0, P=8. span=2. tid=0 gestisce part.orig. 0&1. tid=1 gestisce part.orig. 2&3.
            // Esempio: k=1, P=8. span=4. tid=0 gestisce i blocchi che originariamente erano part. 0,1,2,3.
            long merge_block_span_of_original_partitions = 1 << (k + 1); 

            // Calcola start_index_block1: somma le dimensioni di tutte le partizioni originali
            // che precedono l'inizio del blocco gestito da questo thread.
            start_index_block1 = 0;
            for(int i = 0; i < tid * merge_block_span_of_original_partitions; ++i) {
                 start_index_block1 += base_chunk_size_original + (i < remainder_original ? 1 : 0);
            }

            // 'partitions_in_one_sub_block' indica quante partizioni *originali*
            // compongono UN singolo sotto-blocco che verrà unito.
            // Esempio: k=0, P=8. sub_block_span=1. Il Blocco1 è part.orig.X, Blocco2 è part.orig.Y.
            // Esempio: k=1, P=8. sub_block_span=2. Il Blocco1 è (part.orig.X, part.orig.X+1), Blocco2 è (part.orig.Z, part.orig.Z+1).
            long partitions_in_one_sub_block = 1 << k;

            // Calcola end_index_block1: partendo da start_index_block1, somma le dimensioni
            // delle 'partitions_in_one_sub_block' partizioni originali che formano il primo blocco.
            end_index_block1 = start_index_block1 -1; // Inizializzazione corretta per accumulo
            for(int i = 0; i < partitions_in_one_sub_block; ++i) {
                 // Indice globale della partizione originale i-esima all'interno del primo sotto-blocco
                 int global_original_partition_idx = tid * merge_block_span_of_original_partitions + i;
                 end_index_block1 += base_chunk_size_original + (global_original_partition_idx < remainder_original ? 1 : 0);
            }
            
            start_index_block2 = end_index_block1 + 1; // Il secondo blocco inizia subito dopo il primo.

            // Se start_index_block2 è fuori dall'array, non c'è un secondo blocco da unire.
            // Questo può accadere se N non è grande abbastanza o se P è grande.
            // In questo caso, il worker non esegue il merge, ma parteciperà alla barriera.
            if (start_index_block2 >= N) {
                 DEBUG_PRINT(tid, "[Step %d] Secondo blocco non esiste (start_index_block2=%ld >= N=%ld). Salto logica di merge e copia.", k, start_index_block2, N);
                 // Impostiamo start_index_block2 a N per far saltare la condizione di copia più avanti.
                 // In alternativa, usare un flag booleano 'merge_performed'.
                 start_index_block2 = N; 
                 goto skip_merge_logic_for_this_worker; // Salta direttamente alla barriera.
            }

            // Calcola end_index_block2: partendo da start_index_block2, somma le dimensioni
            // delle 'partitions_in_one_sub_block' partizioni originali che formano il secondo blocco.
            end_index_block2 = start_index_block2 -1; // Inizializzazione corretta per accumulo
            for(int i = 0; i < partitions_in_one_sub_block; ++i) {
                 // Indice globale della partizione originale i-esima all'interno del secondo sotto-blocco
                 int global_original_partition_idx = tid * merge_block_span_of_original_partitions + partitions_in_one_sub_block + i;
                 end_index_block2 += base_chunk_size_original + (global_original_partition_idx < remainder_original ? 1 : 0);
            }
            // Assicura che end_index_block2 non superi la fine dell'array.
            if (end_index_block2 >= N) {
                 end_index_block2 = N - 1;
            }

            DEBUG_PRINT(tid, "[Step %d] INDICI CALCOLATI: Blocco1(%ld-%ld) Blocco2(%ld-%ld)",
                  k, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
            
            // Controllo di sanità: se un blocco ha end < start, è vuoto o c'è un errore.
            if (end_index_block1 < start_index_block1 && N > 0) { // N > 0 per evitare warning su array vuoto
                 DEBUG_PRINT(tid, "[Step %d] ATTENZIONE: Blocco 1 sembra non valido. start1=%ld, end1=%ld", k, start_index_block1, end_index_block1);
            }
            if (end_index_block2 < start_index_block2 && start_index_block2 < N) { // start2 < N per evitare warning se blocco 2 non esiste
                 DEBUG_PRINT(tid, "[Step %d] ATTENZIONE: Blocco 2 sembra non valido. start2=%ld, end2=%ld", k, start_index_block2, end_index_block2);
            }
            
            DEBUG_PRINT(tid, "[Step %d] PRE-MERGE: Unirò array[%ld..%ld] con array[%ld..%ld] in temp_array[%ld..%ld]",
                k, start_index_block1, end_index_block1, start_index_block2, end_index_block2,
                start_index_block1, end_index_block2);

            // --- Inizio Sezione Critica per l'operazione di merge su temp_array ---
            // La mutex serializza le chiamate a merge_sections tra i worker attivi in questo passo.
            // Anche se i worker scrivono (teoricamente) su porzioni disgiunte di temp_array,
            // questa mutex è stata introdotta per maggiore sicurezza o per semplificare la logica
            // evitando di dover garantire la non sovrapposizione in modo più complesso.
            // Se le porzioni sono garantite disgiunte, questa mutex potrebbe essere rimossa per più parallelismo,
            // ma aumenterebbe la complessità di verifica della correttezza degli indici.
            DEBUG_PRINT(tid, "[Step %d] Tentativo di lock merge_mutex...", k);
            pthread_mutex_lock(merge_mutex);
            DEBUG_PRINT(tid, "[Step %d] merge_mutex ACQUISITA. Eseguo merge_sections...", k);

            // Esegue il merge dei due blocchi da 'array' (sorgente) in 'temp_array' (destinazione).
            merge_sections(array, temp_array, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
            
            // Leggi i valori chiave per la stampa DEBUG *dentro* la sezione critica,
            // subito dopo che merge_sections ha (presumibilmente) scritto su temp_array.
            int val_start_read_under_lock = -1; // Valore di default se non letto
            int val_end_read_under_lock = -1;   // Valore di default se non letto
            #if DEBUG
            // Controlla che gli indici siano validi prima di dereferenziare temp_array
            if (start_index_block2 < N) { // Assicura che il merge fosse inteso per accadere
                if (start_index_block1 >= 0 && end_index_block2 >= start_index_block1 && end_index_block2 < N) {
                    val_start_read_under_lock = temp_array[start_index_block1];
                    val_end_read_under_lock = temp_array[end_index_block2];
                }
            }
            #endif
            
            pthread_mutex_unlock(merge_mutex);
            DEBUG_PRINT(tid, "[Step %d] merge_mutex RILASCIATA.",k);
            // --- Fine Sezione Critica ---
            
            // Stampa DEBUG aggiunta per vedere il contenuto di temp_array subito dopo il merge di QUESTO thread.
            #if DEBUG
            // Verifica che gli indici usati per la stampa siano validi.
            if (start_index_block1 >=0 && end_index_block2 >= start_index_block1 && end_index_block2 < N) {
                char temp_label_after_merge[150];
                snprintf(temp_label_after_merge, sizeof(temp_label_after_merge),
                         "[T%d][Step %d] temp_array NOW CONTAINS [%ld-%ld] (immediately after my merge & unlock):",
                         tid, k, start_index_block1, end_index_block2);
                DEBUG_PRINT(tid, "%s temp[%ld]=%d ... temp[%ld]=%d", temp_label_after_merge,
                            start_index_block1, temp_array[start_index_block1],
                            end_index_block2, temp_array[end_index_block2]);
                fflush(stdout);
            }
            #endif

        } else { // Se il worker non è attivo in questo passo.
            DEBUG_PRINT(tid, "[Step %d] INATTIVO (tid=%d >= active_workers=%d).", k, tid, active_workers);
        }

    skip_merge_logic_for_this_worker: // Etichetta per saltare se il secondo blocco non è valido.
        // --- Sincronizzazione 2: Barriera Post-Step di Merge ---
        // Tutti i P worker (attivi e inattivi) si sincronizzano qui.
        // Questo assicura che:
        // 1. Tutti i merge del passo 'k' siano completati e i risultati siano in 'temp_array'.
        // 2. La successiva fase di copia da 'temp_array' ad 'array' inizi solo quando 'temp_array' è stabile.
        DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 2 (post-merge-step)...", k);
        err = pthread_barrier_wait(barrier);
        if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
            DEBUG_PRINT(tid, "[Step %d] Sono l'ultimo thread (serial thread) alla BARRIERA 2.", k);
        #if DEBUG
            // Stampa lo stato di temp_array PRIMA che inizi qualsiasi copia del passo k.
            // A questo punto, tutti i merge_sections del passo k dovrebbero essere completati.
            char label_temp_array_after_all_merges[120];
            snprintf(label_temp_array_after_all_merges, sizeof(label_temp_array_after_all_merges), 
                     "temp_array DOPO TUTTI I MERGE del Passo k=%d (visto da T%d, serial thread barrier)", k, tid);
            print_array(label_temp_array_after_all_merges, temp_array, N); // Stampa temp_array completo
            fflush(stdout);

            // La tua stampa esistente per 'array'
            // char label_array_before_copy[120];
            // snprintf(label_array_before_copy, sizeof(label_array_before_copy), "Array STATO ATTUALE (Passo k=%d, prima delle copie TIDs attivi)", k);
            // print_array(label_array_before_copy, array, N);
            // fflush(stdout);
            #endif
        } else if (err != 0) {
            CHECK_PTHREAD_ERR(err, "Errore fatale in pthread_barrier_wait (barriera post-merge-step)");
        }
        DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 2.", k);

        // --- Fase 3b: Copia del Risultato da temp_array ad array (Post-Barriera) ---
        // Solo i worker che erano attivi in questo passo 'k' e hanno eseguito un merge valido
        // copiano la loro porzione da 'temp_array' nuovamente in 'array'.
        // La condizione 'start_index_block2 < N' assicura che solo chi ha effettivamente unito due blocchi validi faccia la copia.
        if (is_active && start_index_block2 < N) { 
             long copy_s = start_index_block1;    // Indice di inizio per la copia.
             long copy_e = end_index_block2;      // Indice di fine per la copia.
             long num_elements_to_copy_calc = copy_e - copy_s + 1; // Numero di elementi da copiare.

             volatile int *temp_array_debug = t_args->temp_array; //Puntatore volatile per debug
            // Stampe di DEBUG aggiunte per la fase di copia
            DEBUG_PRINT(tid, "[Step %d] PRE-COPY-LOOP: Will copy temp[%ld...%ld] to array[%ld...%ld]. (%ld elementi)",
                        k, copy_s, copy_e, copy_s, copy_e, num_elements_to_copy_calc);
            // Controlla i valori in temp_array *esattamente prima* di iniziare il loop di copia.
            if (copy_s >= 0 && copy_e >= copy_s && copy_e < N) { // Check indici validi per la stampa
                 DEBUG_PRINT(tid, "[Step %d] PRE-COPY-LOOP: current temp_array[%ld]=%d, temp_array[%ld]=%d.",
                             k, copy_s, temp_array[copy_s], copy_e, temp_array[copy_e]);
            } else {
                 DEBUG_PRINT(tid, "[Step %d] PRE-COPY-LOOP: Indici per stampa temp_array non validi copy_s=%ld, copy_e=%ld", k, copy_s, copy_e);
            }
            fflush(stdout);


             // Controllo di validità degli indici di copia.
             if (copy_s < 0 || copy_e >= N || (copy_e < copy_s && num_elements_to_copy_calc > 0) ) {
                 DEBUG_PRINT(tid, "[Step %d] ERRORE COPIA: Indici non validi! start=%ld, end=%ld, N=%ld, num_elem=%ld",
                       k, copy_s, copy_e, N, num_elements_to_copy_calc);
             } else if (num_elements_to_copy_calc <=0 && N > 0) { // N > 0 per evitare warning se N=0
                 DEBUG_PRINT(tid, "[Step %d] COPIA SALTATA: Nessun elemento da copiare. start=%ld, end=%ld, num_elem=%ld",
                       k, copy_s, copy_e, num_elements_to_copy_calc);
             }
             else { // Se gli indici sono validi e ci sono elementi da copiare.
                 DEBUG_PRINT(tid, "[Step %d] DEBUG: Inizio ciclo di copia manuale da temp_array ad array.", k);
                 for (long idx = copy_s; idx <= copy_e; ++idx) {
                     if (idx >= N) { // Ulteriore controllo di sicurezza all'interno del loop.
                        DEBUG_PRINT(tid, "[Step %d] ERRORE BOUNDS LOOP: idx=%ld >= N=%ld durante copia!", k, idx, N);
                        break; 
                     }
                     int val_read_from_temp = temp_array_debug[idx]; // Leggi il valore da temp_array una volta per questo indice.
                     // Stampa DEBUG per ogni elemento copiato (può generare molto output).
                    DEBUG_PRINT(tid, "[Step %d] LOOP-COPY: Leggendo temp_array[%ld] (valore: %d). Scrivendo array[%ld] = %d.",
                    k, idx, val_read_from_temp, idx, val_read_from_temp);

                     array[idx] = val_read_from_temp; // Esegui la copia.
                 }
                 DEBUG_PRINT(tid, "[Step %d] DEBUG: Fine ciclo di copia manuale.");
                 fflush(stdout); // Flush dopo il blocco di copie.
             }
             // Stampa di verifica dopo la copia per questo worker.
            if (copy_s >= 0 && copy_e >= copy_s && copy_e < N) { // Check indici validi
                 DEBUG_PRINT(tid, "[Step %d] POST-COPY-LOOP by T%d: check array[%ld]=%d, array[%ld]=%d.",
                             k, tid, copy_s, array[copy_s], copy_e, array[copy_e]);
                 fflush(stdout);
            }
         }
         DEBUG_PRINT(tid, "--- Fine Passo Merge k=%d ---", k);
         fflush(stdout); // Flush alla fine di ogni passo di merge.
    } // Fine del ciclo for sui passi di merge.

    DEBUG_PRINT(tid, "Fase merge completamente terminata.");
    DEBUG_PRINT(tid, "Worker in terminazione.");
    return NULL; // Termina il thread.
}