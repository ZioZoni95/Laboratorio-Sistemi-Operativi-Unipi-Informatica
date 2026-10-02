#define _POSIX_C_SOURCE 199506L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "wrappers.h"

#define MAX_SLEEP 100000000

// Definizione delle attività possibili di un filosofo
enum activity {
	IS_THINKING,  // Sta pensando
	IS_HUNGRY,    // Ha fame
	IS_EATING     // Sta mangiando
};

static void* philosopher_routine(void*);  // Funzione eseguita da ogni thread filosofo
size_t modulo(int, size_t);               // Funzione modulo per gestire la circolarità

// Variabili globali per la sincronizzazione
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t** condition;                        // Condizioni per gestire il "wait" e il "signal"
enum activity** philosopher_current_activity;     // Stato corrente di ogni filosofo
unsigned int* N;                                  // Numero di filosofi

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <N>\n", argv[0]);
		return 1;
	}

	if (atoi(argv[1]) < 5) {  // Il problema richiede almeno 5 filosofi
		fprintf(stderr, "N is not an integer >= 5.\n");
		return 1;
	}
	unsigned int thread_num = atoi(argv[1]);
	N = &thread_num;

	int err;

	pthread_t* philosopher;
	pthread_cond_t* _stacked_condition;
	enum activity* _stacked_philosopher_current_activity;

	// Allocazione della memoria per i thread e le condizioni
	EXIT_IF_NULL(philosopher, (pthread_t*) malloc(sizeof(pthread_t) * (*N)), "malloc");
	EXIT_IF_NULL(_stacked_condition, (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * (*N)), "malloc");
	EXIT_IF_NULL((_stacked_philosopher_current_activity), (enum activity*) malloc(sizeof(enum activity) * (*N)), "malloc");

	condition = &(_stacked_condition);
	philosopher_current_activity = &(_stacked_philosopher_current_activity);

	// Inizializzazione: ogni filosofo inizia pensando
	for (size_t i = 0; i < *N; i++)
		(*philosopher_current_activity)[i] = IS_THINKING;

	// Inizializzazione delle condizioni per ogni filosofo
	for (size_t i = 0; i < *N; i++)
		EXIT_IF_NOT_EXPECTED_VALUE(err, 0, pthread_cond_init(&((*condition)[i]), NULL), "pthread_cond_init");

	// Creazione dei thread per ogni filosofo
	for (size_t i = 0; i < *N; i++) {
		size_t* pos;
		EXIT_IF_NULL(pos, (size_t*) malloc(sizeof(*pos)), "malloc");
		*pos = i;  // Posizione del filosofo
		Pthread_create(&(philosopher[i]), NULL, &philosopher_routine, (void*) pos);
	}

	// Attendere la terminazione di tutti i thread
	for (size_t i = 0; i < *N; i++) {
		EXIT_IF_NOT_EXPECTED_VALUE(err, 0, pthread_join(philosopher[i], NULL), "pthread_join");
		fprintf(stdout, "(thread) philosopher[%lu] has joined successfully.\n", i);
		fflush(stdout);
	}

	// Liberazione della memoria
	free(*condition);
	free(philosopher);

	return 0;
}

// Funzione modulo per gestire l'indice circolare (es. il vicino sinistro di 0 è N-1)
size_t modulo(int a, size_t mod) {
	if (a < 0) return (size_t) (a + mod);
	else return (size_t) (a % mod);
}

// Routine eseguita da ogni thread (filosofo)
static void* philosopher_routine(void* arg) {
	size_t i = 0;
	int err;
	size_t position = *((size_t*) arg);  // Identificatore del filosofo
	free(arg);
	unsigned int seed = (unsigned int) time(NULL);
	struct timespec delay;

	while (true) {
		// Fase di PENSIERO
		Pthread_mutex_lock(&mutex);
		(*philosopher_current_activity)[position] = IS_THINKING;
		fprintf(stdout, "%lu: philosopher[%lu] is thinking.\n", i, position);
		fflush(stdout);

		// Svegliare i vicini se sono affamati e possono mangiare
		if ((*philosopher_current_activity)[modulo(((int) position) - 1, (*N))] == IS_HUNGRY &&
			(*philosopher_current_activity)[modulo(((int) position) - 2, (*N))] != IS_EATING) {
			(*philosopher_current_activity)[modulo(((int) position) - 1, (*N))] = IS_EATING;
			fprintf(stdout, "Waking up philosopher[%lu]\n", modulo(((int) position) - 1, (*N)));
			pthread_cond_signal(&((*condition)[modulo(((int) position) - 1, (*N))]));
		}

		if ((*philosopher_current_activity)[modulo((int) position + 1, (*N))] == IS_HUNGRY &&
			(*philosopher_current_activity)[modulo((int) position + 2, (*N))] != IS_EATING) {
			(*philosopher_current_activity)[modulo((int) position + 1, (*N))] = IS_EATING;
			fprintf(stdout, "Waking up philosopher[%lu]\n", modulo(((int) position) + 1, (*N)));
			pthread_cond_signal(&((*condition)[modulo((int) position + 1, (*N))]));
		}

		Pthread_mutex_unlock(&mutex);

		if (i == 10) break;  // Dopo 300 cicli il filosofo termina

		// Pausa casuale
		delay.tv_sec = 0;
		delay.tv_nsec = abs(rand_r(&seed) % MAX_SLEEP);
		EXIT_IF_NOT_EXPECTED_VALUE(err, 0, nanosleep(&delay, NULL), "nanosleep");

		// Fase in cui il filosofo ha FAME
		Pthread_mutex_lock(&mutex);
		(*philosopher_current_activity)[position] = IS_HUNGRY;
		fprintf(stdout, "%lu: philosopher[%lu] is hungry.\n", i, position);

		// Aspettare finché i vicini stanno mangiando
		while ((*philosopher_current_activity)[modulo(((int) position) - 1, (*N))] == IS_EATING ||
			   (*philosopher_current_activity)[modulo((int) position + 1, (*N))] == IS_EATING) {
			pthread_cond_wait(&((*condition)[position]), &mutex);
		}

		// Inizio della FASE DI MANGIATA
		(*philosopher_current_activity)[position] = IS_EATING;
		fprintf(stdout, "%lu: philosopher[%lu] is eating.\n", i, position);
		Pthread_mutex_unlock(&mutex);

		// Mangiare per un tempo casuale
		delay.tv_sec = 0;
		delay.tv_nsec = abs(rand_r(&seed) % MAX_SLEEP);
		EXIT_IF_NOT_EXPECTED_VALUE(err, 0, nanosleep(&delay, NULL), "nanosleep");

		fprintf(stdout, "philosopher[%lu] has completed cycle no. %lu\n", position, i + 1);
		i++;
	}

	fprintf(stdout, "philosopher[%lu] has completed their dinner.\n", position);
	pthread_exit((void*) 0);
}
