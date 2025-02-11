#define _POSIX_C_SOURCE 199506L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "wrappers.h"

#define MAX_SLEEP 100000000 // Tempo massimo di "sonno" in nanosecondi per simulare ritardi

// Enum per rappresentare le attività dei filosofi
enum activity { IS_THINKING, IS_HUNGRY, IS_EATING, IS_DONE };

// Dichiarazione delle funzioni
static void* philosopher_routine(void*);
size_t modulo(int, size_t);

// Variabili globali
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex globale per sincronizzazione
pthread_cond_t** condition;                           // Condizioni per il controllo dei filosofi
enum activity** philosopher_current_activity;         // Stato corrente di ogni filosofo
unsigned int* N;                                      // Numero totale di filosofi (thread)

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <N>\n", argv[0]);
        return 1;
    }

    if (atoi(argv[1]) < 5) { // Vincolo minimo sul numero di filosofi
        fprintf(stderr, "N deve essere un intero >= 5.\n");
        return 1;
    }

    unsigned int thread_num = atoi(argv[1]);
    N = &thread_num;

    int err;
    pthread_t* philosopher;
    pthread_cond_t* _stacked_condition;
    enum activity* _stacked_philosopher_current_activity;

    // Allocazione della memoria per i thread e le strutture di sincronizzazione
    EXIT_IF_NULL(philosopher, (pthread_t*) malloc(sizeof(pthread_t) * (*N)), "malloc");
    EXIT_IF_NULL(_stacked_condition, (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * (*N)), "malloc");
    EXIT_IF_NULL(_stacked_philosopher_current_activity, (enum activity*) malloc(sizeof(enum activity) * (*N)), "malloc");

    condition = &(_stacked_condition);
    philosopher_current_activity = &(_stacked_philosopher_current_activity);

    // Inizializzazione dello stato iniziale di ogni filosofo come "pensante"
    for (size_t i = 0; i < *N; i++)
        (*philosopher_current_activity)[i] = IS_THINKING;

    // Inizializzazione delle variabili di condizione per ciascun filosofo
    for (size_t i = 0; i < *N; i++)
        EXIT_IF_NOT_EXPECTED_VALUE(err, 0, pthread_cond_init(&((*condition)[i]), NULL), "pthread_cond_init");

    // Creazione dei thread per i filosofi
    for (size_t i = 0; i < *N; i++) {
        size_t* pos;
        EXIT_IF_NULL(pos, (size_t*) malloc(sizeof(*pos)), "malloc");
        *pos = i;
        printf("DEBUG: Creazione del filosofo %lu\n", i); // Debug
        Pthread_create(&(philosopher[i]), NULL, &philosopher_routine, (void*) pos);
    }

    // Attesa della terminazione di tutti i filosofi
    for (size_t i = 0; i < *N; i++) {
        EXIT_IF_NOT_EXPECTED_VALUE(err, 0, pthread_join(philosopher[i], NULL), "pthread_join");
        printf("(thread) Il filosofo[%lu] ha terminato correttamente.\n", i);
        fflush(stdout);
    }

    // Libera la memoria allocata
    free(*condition);
    free(philosopher);

    return 0;
}

// Funzione modulo per gestire correttamente gli indici circolari
size_t modulo(int a, size_t mod) {
    return (a + mod) % mod;
}

// Routine principale del filosofo
static void* philosopher_routine(void* arg) {
    size_t i = 0;
    int err;
    size_t position = *((size_t*) arg); // Identificatore del filosofo
    free(arg);
    unsigned int seed = (unsigned int) time(NULL); // Seed per la generazione casuale dei tempi di attesa
    struct timespec delay;

    while (i < 5) { // Ogni filosofo esegue 100 cicli di "pensare-mangiare"
        // STATO: PENSARE
        Pthread_mutex_lock(&mutex);
        (*philosopher_current_activity)[position] = IS_THINKING;
        printf("%lu: Il filosofo[%lu] sta pensando.\n", i, position);
        fflush(stdout);

        // Risveglia i vicini affamati se possibile
        for (int offset = -1; offset <= 1; offset += 2) {
            size_t neighbor = modulo((int)position + offset, *N);
            size_t next_neighbor = modulo((int)position + 2 * offset, *N);

            if ((*philosopher_current_activity)[neighbor] == IS_HUNGRY &&
                (*philosopher_current_activity)[next_neighbor] != IS_EATING) {
                (*philosopher_current_activity)[neighbor] = IS_EATING;
                printf("DEBUG: Risveglio del filosofo[%lu]\n", neighbor);
                pthread_cond_signal(&((*condition)[neighbor]));
            }
        }

        Pthread_mutex_unlock(&mutex);

        // Simula il tempo di pensiero
        delay.tv_sec = 0;
        delay.tv_nsec = abs(rand_r(&seed) % MAX_SLEEP);
        EXIT_IF_NOT_EXPECTED_VALUE(err, 0, nanosleep(&delay, NULL), "nanosleep");

        // STATO: AFFAMATO
        Pthread_mutex_lock(&mutex);
        (*philosopher_current_activity)[position] = IS_HUNGRY;
        printf("%lu: Il filosofo[%lu] ha fame.\n", i, position);
        fflush(stdout);

        // Attende finché i vicini non smettono di mangiare (o siano DONE)
        while (((*philosopher_current_activity)[modulo((int)position - 1, *N)] == IS_EATING) ||
               ((*philosopher_current_activity)[modulo((int)position + 1, *N)] == IS_EATING)) {
            printf("Il filosofo[%lu] sta aspettando che i vicini [%lu] e [%lu] finiscano.\n",
                   position, modulo((int)position - 1, *N), modulo((int)position + 1, *N));
            fflush(stdout);
            pthread_cond_wait(&((*condition)[position]), &mutex);
        }

        // STATO: MANGIARE
        (*philosopher_current_activity)[position] = IS_EATING;
        printf("%lu: Il filosofo[%lu] sta mangiando.\n", i, position);
        fflush(stdout);
        Pthread_mutex_unlock(&mutex);

        // Simula il tempo di consumo del pasto
        delay.tv_sec = 0;
        delay.tv_nsec = abs(rand_r(&seed) % MAX_SLEEP);
        EXIT_IF_NOT_EXPECTED_VALUE(err, 0, nanosleep(&delay, NULL), "nanosleep");

        printf("Il filosofo[%lu] ha completato il ciclo n. %lu\n", position, i + 1);
        fflush(stdout);
        i++;
    }

    // Prima di terminare, segnala ai vicini per svegliarli
    Pthread_mutex_lock(&mutex);
    (*philosopher_current_activity)[position] = IS_DONE;
    pthread_cond_signal(&((*condition)[modulo((int)position - 1, *N)]));
    pthread_cond_signal(&((*condition)[modulo((int)position + 1, *N)]));
    Pthread_mutex_unlock(&mutex);

    printf("Il filosofo[%lu] ha finito di cenare.\n", position);
    fflush(stdout);
    pthread_exit((void*) 0);
}
