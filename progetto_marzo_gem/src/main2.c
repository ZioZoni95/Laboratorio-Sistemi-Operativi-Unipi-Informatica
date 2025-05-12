// File: src/main.c
#define _POSIX_C_SOURCE 200809L // Per getopt/optarg

#include "common2.h" // Include structs, externs, pthreads, stdio, ecc.
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> // Per getopt

#include "queue2.h"
#include "master2.h"
#include "worker2.h"

// --- Definizioni delle Variabili Globali ---
// Dichiarate 'extern' in common.h
long* V = NULL;
int N = 0;
int k = 0;

// Funzione per stampare l'usage
void print_usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s -N <array_size> -k <pairs_per_group> -C <queue_capacity> -W <num_workers>\n", prog_name);
    fprintf(stderr, "  -N: Dimensione dell'array (intero > 0)\n");
    fprintf(stderr, "  -k: Numero massimo di coppie per gruppo (intero > 0, k*2 <= N consigliato)\n");
    fprintf(stderr, "  -C: Capacità della coda (intero > 0)\n");
    fprintf(stderr, "  -W: Numero di thread worker (intero >= 1)\n");
}

int main(int argc, char* argv[]) {
    int opt;
    int c_param = 0; // Capacità coda
    int w_param = 0; // Numero worker

    // --- Parsing Argomenti ---
    while ((opt = getopt(argc, argv, "N:k:C:W:")) != -1) {
        // optarg è visibile grazie a _POSIX_C_SOURCE e unistd.h
        switch (opt) {
            case 'N': N = atoi(optarg); break;
            case 'k': k = atoi(optarg); break;
            case 'C': c_param = atoi(optarg); break;
            case 'W': w_param = atoi(optarg); break;
            case '?': print_usage(argv[0]); return EXIT_FAILURE;
            default: abort();
        }
    }

    // --- Validazione Argomenti ---
    if (N <= 0 || k <= 0 || c_param <= 0 || w_param < 1) {
        fprintf(stderr, "Errore: N, k, C devono essere > 0 e W >= 1.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
     if (k*2 > N && N>1) { // Avviso se k è troppo grande per fare gruppi di coppie
         fprintf(stderr, "Attenzione: k*2 (%d) > N (%d). Potrebbero esserci problemi nella divisione in coppie.\n", k*2, N);
     }


    printf("Parametri: N=%d, k=%d, C=%d, W=%d\n", N, k, c_param, w_param);

    // --- Allocazione e Inizializzazione Array V ---
    V = malloc(N * sizeof(long));
    if (V == NULL) { perror("Errore malloc V"); return EXIT_FAILURE; }
    srand(time(NULL)); // Seed per numeri casuali
    long long expected_sum = 0;
    printf("Inizializzazione V con valori 0..99...\n");
    for (int i = 0; i < N; ++i) { V[i] = rand() % 100; expected_sum += V[i]; }
    printf("Inizializzazione V completata. Somma attesa (calcolata da main): %lld\n", expected_sum);

    // --- Inizializzazione Coda Condivisa ---
    SharedQueue queue;
    if (queue_init(&queue, c_param, w_param) != 0) {
        fprintf(stderr, "Errore init coda.\n"); free(V); return EXIT_FAILURE;
    }

    // --- Creazione Argomenti e Thread Worker ---
    pthread_t* worker_tids = malloc(w_param * sizeof(pthread_t));
    WorkerThreadArgs* worker_args = malloc(w_param * sizeof(WorkerThreadArgs));
    if (!worker_tids || !worker_args) {
        perror("Errore malloc TIDs/Args worker");
        free(V); free(worker_tids); free(worker_args);
        queue_destroy(&queue); return EXIT_FAILURE;
    }

    printf("Creazione %d worker...\n", w_param);
    for (int i = 0; i < w_param; ++i) {
        worker_args[i].worker_id = i; // Assegna ID logico
        worker_args[i].queue_ptr = &queue;
        worker_args[i].v_base_ptr = V; // Passa puntatore a V
        worker_args[i].n_size = N;     // Passa N
        if (pthread_create(&worker_tids[i], NULL, worker_routine, &worker_args[i]) != 0) {
            perror("Errore create worker");
             // TODO: Gestione errore più robusta (cancella/join thread già creati)
            free(V); free(worker_tids); free(worker_args);
            queue_destroy(&queue); return EXIT_FAILURE;
        }
        printf("-> Worker %d creato (TID: %lx)\n", i, (unsigned long)worker_tids[i]);
    }

    // --- Esecuzione Logica Master (nel thread main) ---
    printf("\n=== Avvio logica Master nel main ===\n");
    // Passiamo solo la coda, il master accede a V, N, k globali via extern
    // Se volessimo passare più argomenti al master potremmo definire MasterThreadArgs
    master_routine((void*)&queue);
    printf("=== Logica Master nel main completata ===\n\n");

    // --- Attesa Terminazione Worker ---
    printf("Main: Attesa terminazione %d worker...\n", w_param);
    for (int i = 0; i < w_param; ++i) {
        if(pthread_join(worker_tids[i], NULL) != 0) {
            perror("Errore join worker");
            // Potremmo decidere di continuare comunque
        } else {
            // printf("Main: Worker %d (TID: %lx) terminato.\n", i, (unsigned long)worker_tids[i]);
        }
    }
    printf("Main: Tutti i worker hanno terminato.\n");

    // --- Cleanup ---
    printf("Main: Pulizia risorse...\n");
    free(worker_args);
    free(worker_tids);
    free(V);
    queue_destroy(&queue);

    printf("Main: Esecuzione terminata con successo.\n");
    return EXIT_SUCCESS;
}