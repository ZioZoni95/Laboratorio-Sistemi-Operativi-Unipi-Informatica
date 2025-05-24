#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "threads.h"  


void show_menu() {
    int choice;
    printf("==========================================\n");
    printf("     Benvenuto nel programma di Calcolo\n");
    printf("        Somma di un vettore con Thread\n");
    printf("==========================================\n\n");
    printf("Questo programma utilizza un thread Master ed uno o più thread Worker\n");
    printf("per suddividere e elaborare un array di interi in gruppi di coppie.\n\n");
    printf("Modalità disponibili:\n");
    printf(" 1) Modalità Normale\n");
    printf("    - Output essenziale: risultati e info base\n");
    printf(" 2) Modalità Debug\n");
    printf("    - Output dettagliato: informazioni extra su puntatori, stack e sincronizzazione\n");
    printf("Inserisci il numero corrispondente alla modalità scelta: ");
    
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Errore nella lettura della scelta\n");
        exit(EXIT_FAILURE);
    }
    
    switch(choice) {
        case 1:
            debug_mode = 0;
            printf("\nModalità Normale attivata. Uscita del debug extra.\n");
            break;
        case 2:
            debug_mode = 1;
            printf("\nModalità Debug attivata. Verranno visualizzate informazioni extra su puntatori e stack.\n");
            break;
        case 3:
            // Se scegli la modalità info, ad esempio:
            debug_mode = 0;
            printf("\nModalità Info attivata. [Modalità Info non ancora implementata: output minimale con riepilogo]\n");
            break;
        default:
            printf("\nScelta non valida. Modalità Normale attivata per default.\n");
            debug_mode = 0;
    }
    printf("==========================================\n\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("==========================================\n");
        printf("     Benvenuto nel programma di Calcolo\n");
        printf("        Somma di un vettore con Thread\n");
        printf("==========================================\n\n");
        fprintf(stderr, "Uso: %s <num_worker> <N> <k> <capienza_coda>\n", argv[0]);
        printf("  Dove:\n");
        printf("   - <num_worker>:       numero di thread Worker da creare (minimo 1)\n");
        printf("   - <N>:                numero di elementi dell'array da sommare (es: 10)\n");
        printf("   - <k>:                massimo numero di coppie per gruppo (es: 2)\n");
        printf("   - <capienza_coda>:    capacità massima della coda Q condivisa (es: 5)\n\n");
        printf("  Esempio:\n");
        printf("   ./prog_name 2 10 2 5\n");
        printf("   --> Usa 2 Worker, array da 10 elementi, gruppi max da 2 coppie, coda di capacità 5\n");
        printf("==========================================\n\n");
        exit(EXIT_FAILURE);
    }
    
    // Mostra il menu interattivo per scegliere la modalità
    show_menu();
    
    
    num_workers = atoi(argv[1]);
    N = atoi(argv[2]);
    k = atoi(argv[3]);
    capacity = atoi(argv[4]);
    
    if (num_workers < 1 || N < 1 || k < 1 || capacity < 1) {
        fprintf(stderr, "Parametri non validi. Tutti i parametri devono essere >= 1\n");
        exit(EXIT_FAILURE);
    }
    
    // Inizializzazione dell'array (da 1 a N)
    array = malloc(N * sizeof(int));
    if (!array) {
        perror("malloc array");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < N; i++) {
        array[i] = i + 1;
    }
    
    // Inizializza la coda Q con la capacità specificata
    queue_init(&Q, capacity);
    
    pthread_t master_tid;
    pthread_t *worker_tids = malloc(num_workers * sizeof(pthread_t));
    if (!worker_tids) {
        perror("malloc worker_tids");
        exit(EXIT_FAILURE);
    }
    
    // Crea il thread Master
    if (pthread_create(&master_tid, NULL, thread_master, NULL) != 0) {
        perror("pthread_create master");
        exit(EXIT_FAILURE);
    }
    
    // Crea i thread Worker, assegnando a ciascuno il proprio worker_id tramite WorkerArgs
    for (int i = 0; i < num_workers; i++) {
        WorkerArgs *args = malloc(sizeof(WorkerArgs));
        if (!args) {
            perror("malloc WorkerArgs");
            exit(EXIT_FAILURE);
        }
        args->worker_id = i;
        if (pthread_create(&worker_tids[i], NULL, thread_worker, (void *)args) != 0) {
            perror("pthread_create worker");
            exit(EXIT_FAILURE);
        }
    }
    
    // Attende la terminazione del thread Master e di tutti i Worker
    pthread_join(master_tid, NULL);
    for (int i = 0; i < num_workers; i++) {
        pthread_join(worker_tids[i], NULL);
    }
    
    // Pulizia delle risorse: distruzione della coda, rilascio dell'array, e distruzione di mutex e variabili condizione
    queue_destroy(&Q);
    free(worker_tids);
    free(array);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);
    pthread_cond_destroy(&cond_results);
    
    return 0;
}
