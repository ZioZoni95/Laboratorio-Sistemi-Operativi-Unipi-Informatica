/**
 * @file worker.c
 * @brief Implementazione della funzione eseguita dai thread Worker per l'ordinamento parallelo.
 */

#include <math.h>   // Per log2 (anche se calcolato manualmente nel codice)
#include <stdlib.h> // Per qsort
#include <string.h> // Per memcpy (anche se sostituito da loop manuale per debug)

#include "worker.h"
#include "queue.h"   // Per push, pop, close_queue
#include "myutils.h" // Per qsort_compare, merge_sections, print_array

/**
 * @brief Funzione principale eseguita da ciascun thread Worker.
 *
 * Questa funzione implementa l'algoritmo di ordinamento parallelo come descritto
 * nel testo d'esame. Comprende le seguenti fasi:
 * 1. (Solo Worker 0) Calcolo e accodamento delle partizioni iniziali.
 * 2. (Tutti i Worker) Estrazione e ordinamento (qsort) delle partizioni dalla coda.
 * 3. Sincronizzazione tramite barriera.
 * 4. Fase di merge parallelo a passi sincronizzati.
 * 5. Sincronizzazione finale e terminazione.
 *
 * @param args Puntatore a una struttura ThreadArgs contenente gli argomenti per il thread.
 * @return NULL Al termine dell'esecuzione.
 */
void *worker_thread(void *args) {
    // --- 0. Setup Iniziale e Recupero Argomenti ---
    ThreadArgs *t_args = (ThreadArgs *)args;
    int tid = t_args->thread_id;            // ID di questo thread (0 to P-1)
    int P = t_args->n_threads;              // Numero totale di thread Worker (W nel main)
    long N = t_args->n_elements;            // Numero totale di elementi nell'array
    int *array = t_args->array;             // Puntatore all'array condiviso da ordinare
    int *temp_array = t_args->temp_array;   // Puntatore all'array temporaneo per il merge
    ConcurrentQueue *queue = t_args->queue; // Puntatore alla coda concorrente condivisa
    pthread_barrier_t *barrier = t_args->barrier; // Puntatore alla barriera condivisa
    int err;                                // Variabile per controllo errori pthread

    DEBUG_PRINT(tid, "Worker avviato.");

    // ----------------------------------------------------------------------
    // Fase 1: Calcolo e Accodamento Partizioni (Eseguito solo da Worker 0)
    // Testo Esame: "All'avvio, il thread 0 (Worker 0) calcola una serie di
    //              partizioni disgiunte... e ... inserisce nella coda Q..."
    // ----------------------------------------------------------------------
    if (tid == 0) {
        DEBUG_PRINT(tid, "Calcolo e inserimento partizioni iniziali (P=%d)...", P);
        // Calcola la dimensione base di ogni partizione e il resto
        long chunk_size = N / P;
        long remainder = N % P; // Numero di partizioni che avranno un elemento in più
        long current_start = 0; // Indice di partenza per la prossima partizione
        int tasks_pushed = 0;

        // Calcola e inserisce P task (partizioni) nella coda
        for (int i = 0; i < P; ++i) {
            Partition_Index_Task task;
            task.start = current_start;

            // Determina la dimensione di questa specifica partizione
            // Le prime 'remainder' partizioni ottengono un elemento extra
            long current_chunk = chunk_size + (i < remainder ? 1 : 0);

            // Calcola l'indice finale della partizione
            // Se la dimensione è 0 (N < P), end sarà < start
            task.end = current_start + current_chunk - 1;

            // Controllo anti-sforamento (importante se N non è multiplo di P)
            // Nota: Questo controllo è ridondante se il calcolo sopra è corretto,
            // ma è una sicurezza aggiuntiva. task.end dovrebbe essere al massimo N-1.
            // Se current_chunk è 0, task.end sarà < task.start.
            // if (task.end >= N) task.end = N - 1; // Non dovrebbe servire se logica sopra è ok

            // Inserisce il task solo se l'intervallo è valido (start <= end)
            // Questo gestisce il caso N < P, dove alcuni chunk potrebbero essere 0
            if (task.start <= task.end && current_chunk > 0) {
                DEBUG_PRINT(tid, "[Setup] Pushing Task: start=%d, end=%d (chunk=%ld)", task.start, task.end, current_chunk);
                push(queue, task);
                tasks_pushed++;
            } else {
                 DEBUG_PRINT(tid, "[Setup] Skipping Task for partition %d (chunk=%ld, start=%ld, end=%ld)", i, current_chunk, task.start, task.end);
            }
            // Aggiorna l'indice di partenza per la prossima partizione
            current_start += current_chunk;
        }
        DEBUG_PRINT(tid, "Inseriti %d task iniziali. Chiusura coda...", tasks_pushed);
        // Segnala che non verranno aggiunti altri task iniziali alla coda.
        // Questo è necessario per permettere ai worker di terminare il ciclo pop
        // quando la coda diventa vuota.
        close_queue(queue);
    }

    // ----------------------------------------------------------------------
    // Fase 2: Sorting Partizioni (Eseguito da tutti i Worker in parallelo)
    // Testo Esame: "...tutti i Worker prelevano un task alla volta dalla coda
    //              concorrente eseguendo il sorting... (qsort()).
    //              Ogni thread ripete questa operazione finché Q non è vuota..."
    // ----------------------------------------------------------------------
    DEBUG_PRINT(tid, "Inizio fase sorting (ciclo pop/qsort)...");
    Partition_Index_Task current_task;
    int tasks_processed_by_this_thread = 0;
    // Ciclo finché pop() restituisce 1 (cioè, un task è stato estratto con successo)
    // pop() restituirà 0 solo se la coda è vuota E chiusa (da Worker 0).
    while (pop(queue, &current_task)) {
        tasks_processed_by_this_thread++;
        DEBUG_PRINT(tid, "Pop OK: Task(start=%d, end=%d). Eseguo qsort...", current_task.start, current_task.end);

        // Verifica che gli indici siano validi e ci sia almeno un elemento da ordinare
        if (current_task.start <= current_task.end) {
            long num_elements_in_partition = current_task.end - current_task.start + 1;
            qsort(&array[current_task.start],       // Puntatore all'inizio della partizione nell'array principale
                  num_elements_in_partition,        // Numero di elementi nella partizione
                  sizeof(int),                      // Dimensione di ciascun elemento
                  qsort_compare);                   // Funzione di confronto definita in myutils.c
        } else {
            DEBUG_PRINT(tid, "Task(start=%d, end=%d) non valido o vuoto, qsort saltato.", current_task.start, current_task.end);
        }
    }
    // Il ciclo termina perché pop() ha restituito 0.
    DEBUG_PRINT(tid, "Fase sorting terminata. Processati %d task da questo thread.", tasks_processed_by_this_thread);

    // ----------------------------------------------------------------------
    // Sincronizzazione 1: Barriera Post-Sorting
    // Testo Esame: "...quindi tutti i Worker si sincronizzano mediante una BARRIERA,
    //              garantendo che ogni partizione sia stata ordinata prima di
    //              passare alla fase di merge."
    // ----------------------------------------------------------------------
    DEBUG_PRINT(tid, "Attesa su BARRIERA 1 (post-sorting)...");
    err = pthread_barrier_wait(barrier); // Tutti i P thread devono arrivare qui
    // Controllo se questo thread è il "serial thread" (l'ultimo ad arrivare alla barriera)
    // Utile per eseguire azioni una sola volta per ciclo di barriera (qui solo per debug).
    if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
        DEBUG_PRINT(tid, "Sono l'ultimo thread alla BARRIERA 1.");
        // Azioni opzionali da eseguire una sola volta (es. stampare stato intermedio)
        #if DEBUG
        // Stampa l'array dopo che tutte le partizioni iniziali sono state ordinate.
        // Eseguito solo dal "serial thread" per evitare P stampe identiche.
        print_array("Array dopo Fase Sorting (Partizioni ordinate)", array, N);
        #endif
    } else if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
        // Errore imprevisto nella chiamata a pthread_barrier_wait
        CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_wait (post-sort)");
    }
    // Tutti i thread riprendono l'esecuzione *dopo* che P thread hanno chiamato wait.
    DEBUG_PRINT(tid, "Superata BARRIERA 1 (post-sorting).");

    // ----------------------------------------------------------------------
    // Fase 3: Merge Parallelo Sincronizzato
    // Testo Esame: "Superata la barriera, inizia la fase di merge, che viene
    //              implementata in log(base 2) P passi eseguiti in modo sincrono."
    // ----------------------------------------------------------------------
    DEBUG_PRINT(tid, "Inizio fase merge...");

    // Calcola il numero di passi di merge necessari: log2(P)
    // Se P=1, num_steps=0; se P=2, num_steps=1; se P=4, num_steps=2; se P=8, num_steps=3 etc.
    int num_steps = 0;
    if (P > 1) {
        int p_temp = P;
        while (p_temp > 1) {
            p_temp >>= 1; // Divisione intera per 2 (bit shift a destra)
            num_steps++;
        }
    }
    DEBUG_PRINT(tid, "Numero passi di merge necessari: %d", num_steps);

    // Ciclo sui passi di merge (k da 0 a log2(P)-1)
    for (int k = 0; k < num_steps; ++k) {
        DEBUG_PRINT(tid, "--- Inizio Passo Merge k=%d ---", k);

        // Determina quanti worker sono ATTIVI in questo passo e se questo worker lo è.
        // Testo Esame: "Nel passo [k+1] soltanto i primi P/2^(k+1) Worker effettuano il merge..."
        int active_workers = P >> (k + 1); // P / (2^(k+1)) -> P/2, P/4, P/8...
        int is_active = (tid < active_workers); // Questo thread è attivo se il suo ID è minore del numero di attivi

        long start_index_block1 = -1, end_index_block1 = -1;
        long start_index_block2 = -1, end_index_block2 = -1;

        // Solo i worker attivi calcolano gli indici ed eseguono il merge
        if (is_active) {
            DEBUG_PRINT(tid, "[Step %d] ATTIVO (tid=%d < active=%d). Calcolo indici merge...", k, tid, active_workers);

            // Calcolo robusto degli indici dei due blocchi adiacenti da unire.
            // Questi indici si basano sulle dimensioni delle *partizioni iniziali* calcolate da Worker 0.
            long base_chunk_size = N / P;
            long remainder = N % P;

            // Quante partizioni iniziali copre ogni blocco che viene unito IN QUESTO passo k?
            // Esempio: k=0 -> uniamo blocchi di 2^0=1 partizione iniziale. span=2^1=2.
            //          k=1 -> uniamo blocchi di 2^1=2 partizioni iniziali. span=2^2=4.
            //          k=2 -> uniamo blocchi di 2^2=4 partizioni iniziali. span=2^3=8.
            long merge_block_span = 1 << (k + 1); // 2^(k+1)

            // Calcola l'indice di inizio del PRIMO blocco (start_index_block1)
            // Somma le dimensioni di tutte le partizioni iniziali che precedono
            // l'insieme di partizioni gestite da questo thread attivo in questo passo.
            start_index_block1 = 0;
            for(int i = 0; i < tid * merge_block_span; ++i) {
                 start_index_block1 += base_chunk_size + (i < remainder ? 1 : 0);
            }

            // Calcola l'indice di fine del PRIMO blocco (end_index_block1)
            // Parte da start_index_block1 - 1 e somma le dimensioni delle partizioni
            // iniziali che compongono il primo sotto-blocco da unire.
            end_index_block1 = start_index_block1 - 1;
            long partitions_in_sub_block = 1 << k; // 2^k partizioni iniziali per sotto-blocco
            for(int i = 0; i < partitions_in_sub_block; ++i) {
                 // Trova l'indice globale (0..P-1) della partizione iniziale corrente
                 int global_block_idx = tid * merge_block_span + i;
                 // Aggiunge la dimensione di questa partizione iniziale
                 end_index_block1 += base_chunk_size + (global_block_idx < remainder ? 1 : 0);
            }

            // Calcola l'indice di inizio del SECONDO blocco
            start_index_block2 = end_index_block1 + 1;

            // Verifica se il secondo blocco esiste effettivamente nell'array (potrebbe non esistere se N è piccolo o non multiplo di P)
            if (start_index_block2 >= N) {
                 DEBUG_PRINT(tid, "[Step %d] Secondo blocco non esiste (start2=%ld >= N=%ld). Salto merge e copia.", k, start_index_block2, N);
                 start_index_block2 = N; // Segnaliamo che il secondo blocco non è valido per la logica di copia successiva
                 goto skip_merge_logic; // Salta direttamente alla barriera di fine passo
            }

            // Calcola l'indice di fine del SECONDO blocco (end_index_block2)
            // Parte da end_index_block1 e somma le dimensioni delle partizioni
            // iniziali che compongono il secondo sotto-blocco da unire.
            end_index_block2 = end_index_block1; // Inizia da dove finiva il primo blocco
            for(int i = 0; i < partitions_in_sub_block; ++i) {
                 // Trova l'indice globale della partizione iniziale nel secondo sotto-blocco
                 int global_block_idx = tid * merge_block_span + partitions_in_sub_block + i;
                 // Aggiunge la sua dimensione
                 end_index_block2 += base_chunk_size + (global_block_idx < remainder ? 1 : 0);
            }
            // Assicura che l'indice finale non superi i limiti dell'array
            if (end_index_block2 >= N) {
                 end_index_block2 = N - 1;
            }

            // AGGIUNTA DEBUG: Stampa indici calcolati PRIMA del merge effettivo
            DEBUG_PRINT(tid, "[Step %d] INDICI CALCOLATI: Blocco1(%ld-%ld) Blocco2(%ld-%ld)",
                  k, start_index_block1, end_index_block1, start_index_block2, end_index_block2);

            // Controllo di sanità degli indici (debug aggiuntivo)
            if (end_index_block1 < start_index_block1 && N > 0) { // Permetti -1 < 0 se N=0
                 DEBUG_PRINT(tid, "[Step %d] ATTENZIONE: end_index_block1 (%ld) < start_index_block1 (%ld)!", k, end_index_block1, start_index_block1);
            }
             if (end_index_block2 < start_index_block2 && start_index_block2 < N) { // Controlla solo se blocco 2 esiste
                 DEBUG_PRINT(tid, "[Step %d] ATTENZIONE: end_index_block2 (%ld) < start_index_block2 (%ld)!", k, end_index_block2, start_index_block2);
            }

            DEBUG_PRINT(tid, "[Step %d] Eseguo merge: Blocco1 (%ld-%ld), Blocco2 (%ld-%ld) -> temp_array",
                  k, start_index_block1, end_index_block1, start_index_block2, end_index_block2);

            // Esegue il merge leggendo dall'array principale ('array')
            // e scrivendo il risultato ordinato nell'array temporaneo ('temp_array').
            merge_sections(array, temp_array, start_index_block1, end_index_block1, start_index_block2, end_index_block2);
            DEBUG_PRINT(tid, "[Step %d] merge_sections completato.", k);

        } else {
            // Questo thread non è attivo in questo passo di merge
            DEBUG_PRINT(tid, "[Step %d] INATTIVO.", k);
        }

    skip_merge_logic: // Label per saltare qui se il secondo blocco non esiste

        // ------------------------------------------------------------------
        // Sincronizzazione 2: Barriera Post-Step di Merge
        // Testo Esame: "AL termine di ogni step tutti i thread si sincronizzano
        //              nella barriera prima di iniziare lo step successivo"
        // ------------------------------------------------------------------
        DEBUG_PRINT(tid, "[Step %d] Attesa su BARRIERA 2 (post-step)...", k);
        err = pthread_barrier_wait(barrier); // Tutti i P thread devono arrivare qui

        // Ancora, controllo del serial thread e gestione errori barriera
        if (err == PTHREAD_BARRIER_SERIAL_THREAD) {
            DEBUG_PRINT(tid, "[Step %d] Sono l'ultimo thread alla BARRIERA 2.", k);
            // Azioni opzionali da eseguire una sola volta per passo (es. stampare stato)
            #if DEBUG
            // Stampa l'array DOPO la sincronizzazione ma PRIMA della copia da temp ad array.
            // Nota: L'array visualizzato qui potrebbe riflettere GIA' le copie
            // effettuate da ALTRI thread attivi nel passo k, a seconda dello scheduling.
            // Per vedere il risultato *certo* del passo k, bisognerebbe stampare DOPO la copia.
            char label[100];
            snprintf(label, sizeof(label), "Array dopo Sincronizzazione Passo Merge k=%d (Prima della Copia di T%d)", k, tid);
            print_array(label, array, N);
            #endif
        } else if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
            CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_wait (post-merge step)");
        }
        // Tutti i thread riprendono DOPO che P thread hanno chiamato wait.
        DEBUG_PRINT(tid, "[Step %d] Superata BARRIERA 2.", k);


        // ------------------------------------------------------------------
        // Fase 3b: Copia Risultato da temp_array ad array (Post-Barriera)
        // ------------------------------------------------------------------
        // Solo i thread che erano attivi NEL PASSO CORRENTE k e che hanno
        // effettivamente eseguito un merge (cioè il secondo blocco esisteva)
        // copiano il loro risultato dall'array temporaneo all'array principale.
        // La copia avviene DOPO la barriera per assicurare che tutti i merge
        // del passo k siano completati in temp_array prima che qualsiasi
        // thread inizi a leggere da array per il passo k+1.
        if (is_active && start_index_block2 < N) {
             long copy_start = start_index_block1;
             long copy_end = end_index_block2;
             long num_elements_to_copy = copy_end - copy_start + 1;
             // long num_bytes = num_elements_to_copy * sizeof(int); // Non usato nel loop manuale

             DEBUG_PRINT(tid, "[Step %d] COPIO RISULTATO: da temp[%ld...%ld] a array[%ld...%ld] (%ld elementi)",
                   k, copy_start, copy_end, copy_start, copy_end, num_elements_to_copy);

             // Controllo di sicurezza sugli indici prima della copia
             if (copy_start < 0 || copy_end >= N || num_elements_to_copy <= 0) {
                 DEBUG_PRINT(tid, "[Step %d] ERRORE COPIA MANUALE: Indici non validi! start=%ld, end=%ld, N=%ld",
                       k, copy_start, copy_end, N);
             } else {
                 // --- SOSTITUZIONE di memcpy con loop manuale per DEBUG ---
                 // Questa sostituzione è stata fatta per investigare l'errore di ordinamento
                 // osservato con N=8, W=8, dove memcpy sembrava non copiare correttamente.
                 DEBUG_PRINT(tid, "[Step %d] DEBUG: Inizio copia manuale. temp[%ld]=%d, temp[%ld]=%d", k, copy_start, temp_array[copy_start], copy_end, temp_array[copy_end]);
                 for (long idx = copy_start; idx <= copy_end; ++idx) {
                     // Controllo aggiuntivo (non dovrebbe essere necessario se i calcoli sono giusti)
                     if (idx >= N) {
                        DEBUG_PRINT(tid, "[Step %d] ERRORE BOUNDS: idx=%ld >= N=%ld durante copia manuale!", k, idx, N);
                        break;
                     }
                     array[idx] = temp_array[idx]; // Copia elemento per elemento
                 }
                 DEBUG_PRINT(tid, "[Step %d] DEBUG: Fine copia manuale. array[%ld]=%d, array[%ld]=%d", k, copy_start, array[copy_start], copy_end, array[copy_end]);
                 // --- Fine Sostituzione ---

                 // Codice originale commentato:
                 // memcpy(&array[copy_start], &temp_array[copy_start], num_bytes);
             }
         }
         DEBUG_PRINT(tid, "--- Fine Passo Merge k=%d ---", k);
    } // Fine ciclo for sui passi di merge (k)

    DEBUG_PRINT(tid, "Fase merge terminata.");

    // ----------------------------------------------------------------------
    // Fase 4: Terminazione
    // Testo Esame: "...l'intero vettore risulta ordinato ed i thread Worker terminano."
    // L'ultima barriera del ciclo for garantisce che l'ultimo merge (e la copia)
    // sia completato prima che qualsiasi thread termini.
    // ----------------------------------------------------------------------
    DEBUG_PRINT(tid, "Worker terminato.");
    return NULL; // Termina il thread
}