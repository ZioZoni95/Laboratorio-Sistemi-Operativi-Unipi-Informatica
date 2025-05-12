#include <unistd.h> // Per getopt()
#include <time.h>   // Per srand(), time()

// Include i nostri header locali
#include "common.h"
#include "queue.h"
#include "worker.h"
#include "myutils.h"

// Funzione principale del programma
int main(int argc, char *argv[]) {
    long n = 0; // Numero elementi array (N) - Da opzione -n
    int p = 0; // Numero thread worker (P) - Da opzione -p
    int opt; // Variabile per getopt

    DEBUG_PRINT_GEN("Programma avviato.");

    // ----------------------------------------------------------------------
    // Parsing Argomenti Riga di Comando
    // Testo Esame: "Il programma dovrà gestire le seguenti opzioni:
    //              1.Numero di thread Worker (minimo 1).
    //              2.Dimensione dell'array ... (N)."
    // ----------------------------------------------------------------------
    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n': // Opzione -n per il numero di elementi
                n = atol(optarg); // Converte l'argomento in long
                break;
            case 'p': // Opzione -p per il numero di thread
                p = atoi(optarg); // Converte l'argomento in int
                break;
            default: /* '?' o ':' */
                fprintf(stderr, "Uso: %s -n <num_elementi> -p <num_thread>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    DEBUG_PRINT_GEN("Argomenti parsati: N=%ld, P=%d", n, p);

    // Controllo validità argomenti
    if (n <= 0 || p <= 0) {
        fprintf(stderr, "Errore: Specificare -n <num_elementi> (positivo) e -p <num_thread> (positivo).\n");
        exit(EXIT_FAILURE);
    }
    // Controllo se P è potenza di 2 (richiesto dalla logica di merge implementata)
    if ((p > 0) && ((p & (p - 1)) != 0)) {
        fprintf(stderr, "Errore: Il numero di thread P=%d deve essere una potenza di 2 per questa implementazione.\n", p);
        exit(EXIT_FAILURE);
    }

    printf("Avvio parallel_sort con N=%ld elementi e P=%d thread.\n", n, p);

    // ----------------------------------------------------------------------
    // Allocazione Memoria e Inizializzazione Strutture Dati
    // ----------------------------------------------------------------------
    DEBUG_PRINT_GEN("Allocazione memoria per array principale e temporaneo...");
    int *array = malloc(n * sizeof(int));
    CHECK_ERR(array == NULL, "Errore allocazione array principale");
    int *temp_array = malloc(n * sizeof(int)); // Necessario per il merge out-of-place
    CHECK_ERR(temp_array == NULL, "Errore allocazione array temporaneo");
    DEBUG_PRINT_GEN("Allocazione array completata.");

    // Inizializzazione array con numeri casuali
    srand(time(NULL)); // Inizializza generatore casuale
    printf("Inizializzazione array...\n");
    for (long i = 0; i < n; ++i) {
        array[i] = rand() % (n * 10); // Valori casuali in un range ragionevole
    }
    DEBUG_PRINT_GEN("Inizializzazione array completata.");

    #if DEBUG
    print_array("Array Iniziale", array, n); // Stampa array iniziale se in debug
    #endif

    // Inizializzazione Coda Concorrente (Q)
    DEBUG_PRINT_GEN("Inizializzazione Coda Concorrente...");
    ConcurrentQueue queue;
    CHECK_ERR(init_queue(&queue) != 0, "Errore inizializzazione coda");

    // Inizializzazione Barriera Pthreads
    // Testo Esame: "...implementazione della funzione barrier -- ad esempio usando pthread_barrier_wait--"
    DEBUG_PRINT_GEN("Inizializzazione Barriera (per %d threads)...", p);
    pthread_barrier_t barrier;
    int err = pthread_barrier_init(&barrier, NULL, p); // Inizializza per P thread
    CHECK_PTHREAD_ERR(err, "Errore pthread_barrier_init");
    DEBUG_PRINT_GEN("Coda e Barriera inizializzate.");

    // Allocazione strutture per argomenti e ID dei thread
    DEBUG_PRINT_GEN("Allocazione memoria per argomenti e ID thread...");
    ThreadArgs *thread_args = malloc(p * sizeof(ThreadArgs));
    CHECK_ERR(thread_args == NULL, "Errore allocazione ThreadArgs");
    pthread_t *threads = malloc(p * sizeof(pthread_t));
    CHECK_ERR(threads == NULL, "Errore allocazione pthread_t");
    DEBUG_PRINT_GEN("Allocazione completata.");

    // ----------------------------------------------------------------------
    // Creazione Thread Worker
    // ----------------------------------------------------------------------
    printf("Creazione di %d thread worker...\n", p);
    for (int i = 0; i < p; ++i) {
        // Prepara gli argomenti per il thread i
        thread_args[i].thread_id = i;
        thread_args[i].array = array;
        thread_args[i].temp_array = temp_array;
        thread_args[i].n_elements = n;
        thread_args[i].n_threads = p;
        thread_args[i].queue = &queue;
        thread_args[i].barrier = &barrier;
        DEBUG_PRINT_GEN("Creazione thread %d...", i);
        // Crea il thread che eseguirà la funzione 'worker_thread'
        err = pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]);
        CHECK_PTHREAD_ERR(err, "Errore creazione thread");
    }

    // ----------------------------------------------------------------------
    // Attesa Terminazione Thread (Join)
    // ----------------------------------------------------------------------
    printf("Attesa terminazione thread (join)...\n");
    for (int i = 0; i < p; ++i) {
        DEBUG_PRINT_GEN("Join sul thread %d...", i);
        pthread_join(threads[i], NULL); // Attende che il thread i termini
        DEBUG_PRINT_GEN("Join completato per thread %d.", i);
    }
    printf("Tutti i thread hanno terminato.\n");

    #if DEBUG
    print_array("Array Finale", array, n); // Stampa array finale se in debug
    #endif

    // ----------------------------------------------------------------------
    // Verifica Correttezza Ordinamento
    // Testo Esame: "...compila ma non esegue l'ordinamento in modo corretto...,
    //              il codice non verrà valutato." (Implica necessità di verifica)
    // ----------------------------------------------------------------------
    DEBUG_PRINT_GEN("Inizio verifica ordinamento array...");
    int sorted = 1; // Flag per indicare se l'array è ordinato
    for (long i = 0; i < n - 1; ++i) {
        // Controlla se un elemento è maggiore del successivo
        if (array[i] > array[i + 1]) {
            fprintf(stderr, "ERRORE: l'array NON è ordinato! array[%ld]=%d > array[%ld]=%d\n",
                    i, array[i], i + 1, array[i + 1]);
            sorted = 0;
            #if DEBUG // Stampa contesto errore solo in debug
            long start_print = (i > 10) ? i - 10 : 0;
            long end_print = (i + 10 < n) ? i + 10 : n -1;
            fprintf(stderr, "[DEBUG] Elementi intorno all'errore:\n");
            for(long j=start_print; j <= end_print; ++j) {
                fprintf(stderr, "[DEBUG] array[%ld] = %d%s\n", j, array[j], (j==i || j==i+1) ? " <<<" : "");
            }
            #endif
            break; // Esce al primo errore trovato
        }
    }
    // Stampa messaggio finale sulla verifica
    if (sorted) {
        printf("Verifica: L'array è ordinato correttamente.\n");
    } else {
        printf("Verifica: ERRORE, l'array NON è ordinato!\n");
    }
    DEBUG_PRINT_GEN("Verifica ordinamento completata.");

    // ----------------------------------------------------------------------
    // Cleanup Risorse
    // ----------------------------------------------------------------------
    DEBUG_PRINT_GEN("Inizio cleanup risorse...");
    printf("Pulizia risorse...\n");
    free(array);                // Libera memoria array principale
    free(temp_array);           // Libera memoria array temporaneo
    free(thread_args);          // Libera memoria argomenti thread
    free(threads);              // Libera memoria ID thread
    destroy_queue(&queue);      // Distrugge coda (libera nodi, mutex, cond var)
    pthread_barrier_destroy(&barrier); // Distrugge la barriera
    DEBUG_PRINT_GEN("Cleanup completato.");

    printf("Esecuzione terminata con successo.\n");
    return EXIT_SUCCESS; // Termina il programma indicando successo
}