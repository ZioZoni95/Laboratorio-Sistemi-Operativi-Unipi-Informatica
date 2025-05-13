/**
 * @file main.c
 * @brief Programma principale per l'ordinamento parallelo di un array.
 *
 * Gestisce il parsing degli argomenti, l'allocazione delle risorse,
 * la creazione e la gestione dei thread worker, la verifica finale
 * e il cleanup. Introduce una mutex per serializzare le operazioni
 * di merge su temp_array e una nuova mutex per la fase di copia.
 */

#include <unistd.h>  
#include <time.h>    
#include <pthread.h>

#include "common.h"
#include "queue.h"
#include "worker.h"
#include "myutils.h"

// Dichiarazione della mutex globale per la fase di merge su temp_array
pthread_mutex_t merge_temp_array_mutex;

// Dichiarazione della mutex globale per la fase di copia da temp_array ad array
pthread_mutex_t copy_phase_mutex;


int main(int argc, char *argv[]) {
    long n = 0; // Numero elementi array (N) - Da opzione -n
    int p = 0;  // Numero thread worker (P) - Da opzione -w
    int opt;    // Variabile per getopt
    int err;    // Variabile per controllo errori pthread

    // --- Parsing Argomenti Riga di Comando ---
    // Utilizza getopt per leggere le opzioni -n (numero elementi) e -w (numero worker)
    while ((opt = getopt(argc, argv, "n:w:")) != -1) {
        switch (opt) {
            case 'n':
                n = atol(optarg); // Converte l'argomento di -n a long
                break;
            case 'w':
                p = atoi(optarg); // Converte l'argomento di -w a int
                break;
            default:
                // Se viene usata un'opzione non valida, stampa un messaggio di errore ed esce
                fprintf(stderr, "Uso: %s -n <num_elementi> -w <num_worker>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // --- Controllo Validità Argomenti ---
    if (n <= 0 || p <= 0) {
        fprintf(stderr, "Errore: Specificare -n <num_elementi> (positivo) e -w <num_worker> (positivo).\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    // Controllo se P è una potenza di 2.
    // p = 1 è una potenza di 2 (2^0), quindi è valido.
    // Per p > 1, (p & (p - 1)) == 0 se p è una potenza di 2.
    // Quindi, (p & (p - 1)) != 0 se p NON è una potenza di 2 (per p > 1).
    if ((p != 1) && ((p & (p - 1)) != 0)) {
        fprintf(stderr, "ERRORE: Il numero di worker P=%d (da opzione -w) non è una potenza di 2.\n", p);
        fprintf(stderr, "         Per questo algoritmo, P deve essere una potenza di 2 (es. 1, 2, 4, 8, ...).\n");
        exit(EXIT_FAILURE); // Interrompe l'esecuzione 
    }

    printf("Avvio parallel_sort con N=%ld elementi e P=%d worker (da -w).\n", n, p);

    // --- Allocazione Memoria ---
    // Alloca memoria per l'array principale che conterrà i dati da ordinare
    int *array = malloc(n * sizeof(int));
    CHECK_ERR(array == NULL, "Errore allocazione array principale"); // Controlla se malloc ha fallito

    // Alloca memoria per l'array temporaneo usato durante la fase di merge
    int *temp_array = malloc(n * sizeof(int));
    CHECK_ERR(temp_array == NULL, "Errore allocazione array temporaneo");

   
    // --- Inizializzazione Array Casuale ---
    srand(time(NULL)); // Inizializza il generatore di numeri casuali
    printf("Inizializzazione array con valori casuali...\n");
    for (long i = 0; i < n; ++i) {
        array[i] = rand() % (n * 10); // Riempie l'array con valori casuali
    }

    // Stampa l'array iniziale se DEBUG è 0 (richiesta specifica)
    // o se DEBUG è diverso da 0 (comportamento standard della macro DEBUG_PRINT)
    #if DEBUG == 0
    if (n > 0) print_array("Array Iniziale", array, n);
    #else
    print_array("Array Iniziale (DEBUG ATTIVO)", array, n); // Questa è la stampa originale sotto #if DEBUG
    #endif


    // --- Inizializzazione Strutture di Sincronizzazione ---
    ConcurrentQueue queue; // Istanza della coda concorrente
    CHECK_ERR(init_queue(&queue) != 0, "Errore inizializzazione coda"); // Inizializza la coda

    pthread_barrier_t barrier; // Istanza della barriera
    // Inizializza la barriera per 'p' thread worker
    err = pthread_barrier_init(&barrier, NULL, p);
    CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_init");

    // Inizializza il mutex usato per proteggere l'accesso a temp_array durante il merge
    err = pthread_mutex_init(&merge_temp_array_mutex, NULL);
    CHECK_PTHREAD_ERR(err, "Errore pthread_mutex_init for merge_temp_array_mutex");

    // Inizializza il mutex usato per proteggere la copia da temp_array ad array
    err = pthread_mutex_init(&copy_phase_mutex, NULL);
    CHECK_PTHREAD_ERR(err, "Errore pthread_mutex_init for copy_phase_mutex");



    // --- Preparazione Argomenti Thread ---
    // Alloca memoria per un array di strutture ThreadArgs, una per ogni worker
    ThreadArgs *thread_args = malloc(p * sizeof(ThreadArgs));
    CHECK_ERR(thread_args == NULL, "Errore allocazione ThreadArgs");
    // Alloca memoria per un array di identificatori di thread (pthread_t)
    pthread_t *threads = malloc(p * sizeof(pthread_t));
    CHECK_ERR(threads == NULL, "Errore allocazione pthread_t");

    // --- Creazione Thread Worker ---
    printf("Creazione di %d thread worker (da -w)...\n", p);
    for (int i = 0; i < p; ++i) {
        // Popola la struttura ThreadArgs per il worker corrente
        thread_args[i].thread_id = i;
        thread_args[i].array = array;
        thread_args[i].temp_array = temp_array;
        thread_args[i].n_elements = n;
        thread_args[i].n_threads = p;
        thread_args[i].queue = &queue;
        thread_args[i].barrier = &barrier;
        thread_args[i].merge_mutex_ptr = &merge_temp_array_mutex;
        thread_args[i].copy_phase_mutex_ptr = &copy_phase_mutex;
        
        // Crea il thread worker, passando la funzione worker_thread e gli argomenti specifici
        err = pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]);
        CHECK_PTHREAD_ERR(err, "Errore creazione thread");
    }

    // --- Attesa Terminazione Thread (Join) ---
    // Il thread principale attende che tutti i thread worker terminino la loro esecuzione.
    printf("Attesa terminazione thread (join)...\n");
    for (int i = 0; i < p; ++i) {
        pthread_join(threads[i], NULL); // Attende la terminazione del thread i-esimo
    }
    printf("Tutti i thread hanno terminato.\n");

    // Stampa l'array finale se DEBUG è 0 
    // o se DEBUG è diverso da 0 (comportamento standard della macro DEBUG_PRINT)
    #if DEBUG == 0
    if (n > 0) print_array("Array Finale", array, n);
    #else
    print_array("Array Finale (DEBUG ATTIVO)", array, n); // Questa è la stampa originale sotto #if DEBUG
    #endif


    // --- Verifica Correttezza Ordinamento ---
    DEBUG_PRINT_GEN("Inizio verifica ordinamento array...");
    int sorted = 1; // Flag per indicare se l'array è ordinato
    // Scorre l'array per verificare se è ordinato confrontando elementi adiacenti
    for (long i = 0; i < n - 1; ++i) {
        if (array[i] > array[i + 1]) {
            fprintf(stderr, "ERRORE: l'array NON è ordinato! array[%ld]=%d > array[%ld]=%d\n",
                    i, array[i], i + 1, array[i + 1]);
            // Se DEBUG è attivo e si trova un errore, stampa una porzione dell'array attorno all'errore
            #if DEBUG
            long start_print = (i > 10) ? i - 10 : 0;
            long end_print = (i + 10 < n) ? i + 10 : n -1;
            if (n > 0) {
                fprintf(stderr, "[DEBUG] Elementi intorno all'errore (indici %ld-%ld):\n", start_print, end_print);
                for(long j = start_print; j <= end_print; ++j) {
                    fprintf(stderr, "[DEBUG] array[%ld] = %d%s\n", j, array[j], (j==i || j==i+1) ? " <<< ERRORE QUI" : "");
                }
            }
            #endif
            sorted = 0; // Imposta il flag a 0 (non ordinato)
            break;      // Interrompe il ciclo, non è necessario continuare
        }
    }
    if (sorted) {
        printf("Verifica: L'array è ordinato correttamente.\n");
    } else {
        printf("Verifica: ERRORE, l'array NON è ordinato!\n");
    }
    DEBUG_PRINT_GEN("Verifica ordinamento completata.");

    // --- Cleanup Risorse ---
    // Libera tutta la memoria allocata dinamicamente e distrugge le primitive di sincronizzazione.
    DEBUG_PRINT_GEN("Inizio cleanup risorse...");
    printf("Pulizia risorse...\n");
    free(array);                      // Libera l'array principale
    free(temp_array);                 // Libera l'array temporaneo
    free(thread_args);                // Libera l'array degli argomenti dei thread
    free(threads);                    // Libera l'array degli ID dei thread
    destroy_queue(&queue);            // Distrugge la coda concorrente
    pthread_barrier_destroy(&barrier); // Distrugge la barriera
    pthread_mutex_destroy(&merge_temp_array_mutex); // Distrugge il mutex di merge
    pthread_mutex_destroy(&copy_phase_mutex);       // Distrugge il mutex di copia
    DEBUG_PRINT_GEN("Mutex per fase di copia distrutto.");
    DEBUG_PRINT_GEN("Cleanup completato.");

    printf("Esecuzione terminata con successo.\n");
    return EXIT_SUCCESS; // Termina il programma con successo
}