/**
 * @file main.c
 * @brief Programma principale per l'ordinamento parallelo di un array.
 *
 * Gestisce il parsing degli argomenti, l'allocazione delle risorse,
 * la creazione e la gestione dei thread worker, la verifica finale
 * e il cleanup. Introduce una mutex per serializzare le operazioni
 * di merge su temp_array.
 */

#include <unistd.h> // Per getopt()
#include <time.h>   // Per srand(), time()
#include <pthread.h> // Per pthread_mutex_t

// Include i nostri header locali
#include "common.h"
#include "queue.h"
#include "worker.h"
#include "myutils.h"

// Dichiarazione della mutex globale per la fase di merge su temp_array
pthread_mutex_t merge_temp_array_mutex;

// Funzione principale del programma
int main(int argc, char *argv[]) {
    long n = 0; // Numero elementi array (N) - Da opzione -n
    int p = 0; // Numero thread worker (P) - Da opzione -w
    int opt; // Variabile per getopt
    int err; // Variabile per controllo errori pthread

    DEBUG_PRINT_GEN("Programma avviato.");

    // --- Parsing Argomenti Riga di Comando ---
    while ((opt = getopt(argc, argv, "n:w:")) != -1) {
        switch (opt) {
            case 'n':
                n = atol(optarg);
                break;
            case 'w':
                p = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Uso: %s -n <num_elementi> -w <num_worker>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    DEBUG_PRINT_GEN("Argomenti parsati: N=%ld, P=%d (da -w)", n, p);

    // --- Controllo Validità Argomenti ---
    if (n <= 0 || p <= 0) {
        fprintf(stderr, "Errore: Specificare -n <num_elementi> (positivo) e -w <num_worker> (positivo).\n");
        exit(EXIT_FAILURE);
    }
    // NOTA: La traccia originale non impone P come potenza di 2, ma l'algoritmo di merge a passi log2(P)
    //       con dimezzamento dei worker funziona più naturalmente (e spesso è implementato) con P potenza di 2.
    //       Se la tua logica di calcolo indici lo richiede, mantieni questo controllo.
    if ((p > 0) && (p != 1) && ((p & (p - 1)) != 0)) { // p=1 è un caso base valido
        fprintf(stderr, "Avviso: Il numero di worker P=%d (da -w) non è una potenza di 2. La logica di merge potrebbe non funzionare come previsto per tutti i valori di P.\n", p);
        // Potresti voler terminare con EXIT_FAILURE se la tua implementazione lo richiede strettamente.
        // exit(EXIT_FAILURE);
    }

    printf("Avvio parallel_sort con N=%ld elementi e P=%d worker (da -w).\n", n, p);

    // --- Allocazione Memoria ---
    DEBUG_PRINT_GEN("Allocazione memoria per array principale e temporaneo...");
    int *array = malloc(n * sizeof(int));
    CHECK_ERR(array == NULL, "Errore allocazione array principale");

    int *temp_array = malloc(n * sizeof(int));
    CHECK_ERR(temp_array == NULL, "Errore allocazione array temporaneo");

    // !!! MODIFICA SUGGERITA: Inizializzazione di temp_array !!!
    // Questo aiuta a distinguere tra valori non inizializzati e valori corrotti/errati.
    // Se vedi zeri dove ti aspetti altri numeri, le scritture non sono avvenute.
    // Se vedi ancora numeri casuali grandi, la memoria è stata sovrascritta.
    DEBUG_PRINT_GEN("Inizializzazione temp_array a 0...");
    for (long i = 0; i < n; ++i) {
        temp_array[i] = 0; // o un altro valore di debug, es. -1
    }
    DEBUG_PRINT_GEN("Allocazione e inizializzazione temp_array completata.");


    // --- Inizializzazione Array Casuale ---
    srand(time(NULL)); // Per la generazione di numeri casuali
    printf("Inizializzazione array con valori casuali...\n");
    for (long i = 0; i < n; ++i) {
        array[i] = rand() % (n * 10); // Valori casuali, ad esempio, tra 0 e N*10-1
    }
    DEBUG_PRINT_GEN("Inizializzazione array con valori casuali completata.");
    #if DEBUG
    print_array("Array Iniziale", array, n);
    #endif

    // --- Inizializzazione Strutture di Sincronizzazione ---
    DEBUG_PRINT_GEN("Inizializzazione Coda Concorrente...");
    ConcurrentQueue queue;
    CHECK_ERR(init_queue(&queue) != 0, "Errore inizializzazione coda");

    DEBUG_PRINT_GEN("Inizializzazione Barriera (per %d worker)...", p);
    pthread_barrier_t barrier;
    // Il numero di thread per la barriera deve essere P (tutti i worker)
    err = pthread_barrier_init(&barrier, NULL, p);
    CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_init");

    DEBUG_PRINT_GEN("Inizializzazione Mutex per Merge...");
    err = pthread_mutex_init(&merge_temp_array_mutex, NULL);
    CHECK_PTHREAD_ERR(err, "Errore pthread_mutex_init for merge_temp_array_mutex");

    DEBUG_PRINT_GEN("Coda, Barriera e Mutex Merge inizializzate.");

    // --- Preparazione Argomenti Thread ---
    DEBUG_PRINT_GEN("Allocazione memoria per argomenti e ID thread...");
    ThreadArgs *thread_args = malloc(p * sizeof(ThreadArgs));
    CHECK_ERR(thread_args == NULL, "Errore allocazione ThreadArgs");
    pthread_t *threads = malloc(p * sizeof(pthread_t));
    CHECK_ERR(threads == NULL, "Errore allocazione pthread_t");
    DEBUG_PRINT_GEN("Allocazione completata.");

    // --- Creazione Thread Worker ---
    printf("Creazione di %d thread worker (da -w)...\n", p);
    for (int i = 0; i < p; ++i) {
        thread_args[i].thread_id = i;
        thread_args[i].array = array;
        thread_args[i].temp_array = temp_array; // Tutti i thread puntano allo stesso temp_array
        thread_args[i].n_elements = n;
        thread_args[i].n_threads = p;
        thread_args[i].queue = &queue;
        thread_args[i].barrier = &barrier;
        thread_args[i].merge_mutex_ptr = &merge_temp_array_mutex;
        DEBUG_PRINT_GEN("Creazione thread %d...", i);
        err = pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]);
        CHECK_PTHREAD_ERR(err, "Errore creazione thread");
    }

    // --- Attesa Terminazione Thread (Join) ---
    printf("Attesa terminazione thread (join)...\n");
    for (int i = 0; i < p; ++i) {
        DEBUG_PRINT_GEN("Join sul thread %d...", i);
        pthread_join(threads[i], NULL);
        DEBUG_PRINT_GEN("Join completato per thread %d.", i);
    }
    printf("Tutti i thread hanno terminato.\n");

    #if DEBUG
    print_array("Array Finale", array, n);
    #endif

    // --- Verifica Correttezza Ordinamento ---
    DEBUG_PRINT_GEN("Inizio verifica ordinamento array...");
    int sorted = 1;
    for (long i = 0; i < n - 1; ++i) {
        if (array[i] > array[i + 1]) {
            fprintf(stderr, "ERRORE: l'array NON è ordinato! array[%ld]=%d > array[%ld]=%d\n",
                    i, array[i], i + 1, array[i + 1]);
            #if DEBUG // Stampa una porzione dell'array intorno all'errore per debug
            long start_print = (i > 10) ? i - 10 : 0;
            long end_print = (i + 10 < n) ? i + 10 : n -1; // Assicura di non andare fuori limiti
            if (n > 0) { // Evita problemi con n=0
                fprintf(stderr, "[DEBUG] Elementi intorno all'errore (indici %ld-%ld):\n", start_print, end_print);
                for(long j = start_print; j <= end_print; ++j) {
                    fprintf(stderr, "[DEBUG] array[%ld] = %d%s\n", j, array[j], (j==i || j==i+1) ? " <<< ERRORE QUI" : "");
                }
            }
            #endif
            sorted = 0;
            break;
        }
    }
    if (sorted) {
        printf("Verifica: L'array è ordinato correttamente.\n");
    } else {
        printf("Verifica: ERRORE, l'array NON è ordinato!\n");
    }
    DEBUG_PRINT_GEN("Verifica ordinamento completata.");

    // --- Cleanup Risorse ---
    DEBUG_PRINT_GEN("Inizio cleanup risorse...");
    printf("Pulizia risorse...\n");
    free(array);
    free(temp_array);
    free(thread_args);
    free(threads);
    destroy_queue(&queue);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&merge_temp_array_mutex);
    DEBUG_PRINT_GEN("Cleanup completato.");

    printf("Esecuzione terminata con successo.\n");
    return EXIT_SUCCESS;
}