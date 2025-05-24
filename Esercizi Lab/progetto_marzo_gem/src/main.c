// File: src/main.c
#include "common.h" // Il nostro header comune
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Per getopt
#include <errno.h>  // Per perror
#include <time.h>   // Per srand (se inizializziamo V casualmente)
#include <bits/getopt_core.h>
#include "queue.h"  // Funzioni della coda (init, destroy, push, pop)
#include "master.h" // Prototipo per master_routine
#include "worker.h" // Prototipo per worker_routine

// Variabili globali per i parametri (o passate tramite struct al master)
// Per ora globali per semplicità nello scheletro
long* V = NULL;
int N = 0; // Dimensione array
int k = 0; // Max coppie per gruppo
int C = 0; // Capacità coda
int W = 0; // Numero worker

// Funzione per stampare l'usage
void print_usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s -N <array_size> -k <pairs_per_group> -C <queue_capacity> -W <num_workers>\n", prog_name);
    fprintf(stderr, "  -N: Dimensione dell'array (intero > 0)\n");
    fprintf(stderr, "  -k: Numero massimo di coppie per gruppo (intero > 0)\n");
    fprintf(stderr, "  -C: Capacità della coda (intero > 0)\n");
    fprintf(stderr, "  -W: Numero di thread worker (intero >= 1)\n");
}

int main(int argc, char* argv[]) {
    int opt;
    int i;

    // --- PASSO 1: Parsing Argomenti Riga di Comando ---
    // L'appello richiede di gestire N, k, C, Numero Worker (W) come opzioni
    while ((opt = getopt(argc, argv, "N:k:C:W:")) != -1) {
        switch (opt) {
            case 'N':
                N = atoi(optarg); // Converti stringa a intero
                break;
            case 'k':
                k = atoi(optarg);
                break;
            case 'C':
                C = atoi(optarg);
                break;
            case 'W':
                W = atoi(optarg);
                break;
            case '?': // Opzione non riconosciuta o argomento mancante
                print_usage(argv[0]);
                return EXIT_FAILURE;
            default:
                abort(); // Non dovrebbe succedere
        }
    }

    // --- PASSO 2: Validazione Argomenti ---
    if (N <= 0 || k <= 0 || C <= 0 || W < 1) {
        fprintf(stderr, "Errore: Tutti i parametri (N, k, C, W) devono essere positivi (W >= 1).\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    printf("Parametri: N=%d, k=%d, C=%d, W=%d\n", N, k, C, W);

    // --- PASSO 3: Allocazione Array V ---
    V = malloc(N * sizeof(long));
    if (V == NULL) {
        perror("Errore malloc array V");
        return EXIT_FAILURE;
    }
    // Inizializzazione Array V (esempio: numeri da 1 a N)
    printf("Inizializzazione array V...\n");
    srand(time(NULL));
    long long expected_sum = 0; // Calcoliamo la somma attesa per verifica
    for (i = 0; i < N; ++i) {
        V[i] = i; //parto dall'indice i si può fare anche in altri indici,anche casuali
        expected_sum += V[i];
    }
    printf("Array V inizializzato. Somma attesa: %lld\n", expected_sum);

    // --- PASSO 4: Inizializzazione Coda Condivisa ---
    SharedQueue queue;
    //Chiama la funzione di init per la coda
    if(queue_init(&queue, C,W) != 0){
        fprintf(stderr, "Errore: impossibile inizializzare la coda condivisa.\n");
        free(V);
        return EXIT_FAILURE;
    }
    //Se queue_init return 0 -> Coda Pronta all'uso
    
    // --- PASSO 5: Creazione Thread Worker ---
    pthread_t* worker_tids = malloc(W * sizeof(pthread_t));
    if (worker_tids == NULL) {
        perror("Errore malloc worker_tids");
        free(V); // Libera memoria già allocata
        queue_destroy(&queue); //Chiama la distruzione in caso di errore
        return EXIT_FAILURE;
    }
    printf("Creazione di %d thread worker...\n", W);
    for (i = 0; i < W; ++i) {
        // Crea un worker thread, eseguirà worker_routine
        // Passiamo il puntatore alla coda condivisa come argomento
        if (pthread_create(&worker_tids[i], NULL, worker_routine, (void*)&queue) != 0) {
            // Errore creazione thread i-esimo
            perror("Errore pthread_create worker");
            // Gestione errore più robusta: dovremmo cancellare/joinare i thread già creati
            // Per semplicità qui terminiamo dopo aver liberato memoria
            free(V);
            free(worker_tids);
            queue_destroy(&queue);
            return EXIT_FAILURE;
        }
        printf("Worker %d creato (TID: %lx)\n", i, (unsigned long)worker_tids[i]);
    }
    printf("Tutti i worker sono stati creati.\n");

    // --- PASSO 6: Esecuzione Logica Master ---
    // Decidiamo di eseguire la logica Master direttamente nel thread main
    // dopo aver creato i worker. Chiamiamo master_routine.
    // Il puntatore alla coda è già disponibile nella variabile 'queue'.
    printf("Avvio logica Master nel thread main...\n");
    master_routine((void*)&queue); // Chiamiamo la routine del master
    printf("Logica Master completata nel thread main.\n");

    // --- PASSO 7: Attesa Terminazione Worker ---
    // Il Master (thread main) deve attendere che tutti i worker abbiano finito.
    printf("Attesa terminazione thread worker...\n");
    for (i = 0; i < W; ++i) {
        // pthread_join attende che il thread specificato termini
        // Il secondo argomento (NULL qui) potrebbe ricevere il valore restituito dal thread
        if (pthread_join(worker_tids[i], NULL) != 0) {
            perror("Errore pthread_join worker");
            // Continuiamo comunque a joinare gli altri e a fare cleanup
        } else {
            printf("Worker %d terminato (TID: %lx).\n", i, (unsigned long)worker_tids[i]);
        }
    }
    printf("Tutti i worker hanno terminato.\n");

    // --- PASSO 8: Cleanup ---
    printf("Pulizia risorse...\n");
    free(worker_tids);
    free(V);
    queue_destroy(&queue);
    // TODO: Chiamare queue_destroy(&queue); per liberare buffer coda e distruggere mutex/CV

    printf("Esecuzione terminata.\n");
    return EXIT_SUCCESS;
}