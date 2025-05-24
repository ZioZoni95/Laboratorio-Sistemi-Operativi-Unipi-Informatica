#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define SENTINEL -1

typedef struct {
    int *buffer;
    int capacity;
    int size;
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t isFull;
    pthread_cond_t isEmpty;
} Queue;

// Funzioni per la coda
void queue_init(Queue *q, int capacity) {
    q->buffer = (int *)malloc(capacity * sizeof(int));
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->isFull, NULL);
    pthread_cond_init(&q->isEmpty, NULL);
}

void queue_push(Queue *q, int value) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == q->capacity) {
        pthread_cond_wait(&q->isFull, &q->mutex);
    }
    q->buffer[q->rear] = value;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
    pthread_cond_signal(&q->isEmpty);
    pthread_mutex_unlock(&q->mutex);
}

int queue_pop(Queue *q) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == 0) {
        pthread_cond_wait(&q->isEmpty, &q->mutex);
    }
    int value = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    pthread_cond_signal(&q->isFull);
    pthread_mutex_unlock(&q->mutex);
    return value;
}

// Variabili globali
int *array;
int N, k, C, NUM_WORKER;
Queue queue;
int sommaFinale = 0;
int workerAttivi = 0;
pthread_mutex_t mutexSomma = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condWorkerFinito = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexPrint = PTHREAD_MUTEX_INITIALIZER;
int workerIDs[100];

// **Master Thread**
void *T_Master(void *arg) {
    printf("[Master] Inizio produzione gruppi...\n");

    int i = 0;
    while (i < N) {
        int size = ((i + 2 * k) <= N) ? (2 * k) : (N - i);

        printf("[Master] Gruppo (size: %d): ", size);
        for (int j = 0; j < size; j++) {
            printf("%d ", array[i + j]);
            queue_push(&queue, array[i + j]);
        }
        printf("\n");

        queue_push(&queue, SENTINEL);
        i += size;
    }

    // **Segnalo ai Worker che il lavoro è finito**
    for (int i = 0; i < NUM_WORKER; i++) {
        queue_push(&queue, SENTINEL);
    }

    // **Aspetto che tutti i Worker abbiano finito**
    pthread_mutex_lock(&mutexSomma);
    while (workerAttivi > 0) {
        pthread_cond_wait(&condWorkerFinito, &mutexSomma);
    }
    pthread_mutex_unlock(&mutexSomma);

    printf("[Master] Somma totale finale: %d\n", sommaFinale);
    pthread_exit(NULL);
}

// **Worker Thread**
void *T_Worker(void *arg) {
    int workerID = *(int *)arg;  // Ricavo l'ID del Worker
    free(arg);  // Libero la memoria allocata

    pthread_mutex_lock(&mutexSomma);
    workerAttivi++;
    pthread_mutex_unlock(&mutexSomma);

    int sommaParziale = 0;

    while (1) {
        int gruppo[2 * k];
        int size = 0; 

        while (size < 2 * k) {
            int valore = queue_pop(&queue);
            if (valore == SENTINEL) break;
            gruppo[size++] = valore;
        }

        if (size == 0) break;

        pthread_mutex_lock(&mutexPrint);
        printf("[Worker %d] Processando gruppo:", workerID);
        pthread_mutex_unlock(&mutexPrint);

        int sommaGruppo = 0;
        for (int i = 0; i < size; i++) {
            printf(" %d", gruppo[i]);
            sommaGruppo += gruppo[i];
        }
        printf("\n");

        sommaParziale += sommaGruppo;
        printf("[Worker %d] Somma gruppo: %d, Somma accumulata: %d\n", 
               workerID, sommaGruppo, sommaParziale);
    }

    pthread_mutex_lock(&mutexSomma);
    sommaFinale += sommaParziale;
    workerAttivi--;
    if (workerAttivi == 0) {
        pthread_cond_signal(&condWorkerFinito);
    }
    pthread_mutex_unlock(&mutexSomma);

    pthread_exit(NULL);
}

// **Main**
int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s <N> <k> <C> <NUM_WORKER>\n", argv[0]);
        return 1;
    }
    N = atoi(argv[1]);
    k = atoi(argv[2]);
    C = atoi(argv[3]);
    NUM_WORKER = atoi(argv[4]);

    array = malloc(N * sizeof(int));
    printf("[Main] Array generato: ");
    for (int i = 0; i < N; i++) {
        array[i] = rand() % 10 + 1;
        printf("%d ", array[i]);
    }
    printf("\n");

    queue_init(&queue, C);

    pthread_t master;
    pthread_t workers[NUM_WORKER];

    pthread_create(&master, NULL, T_Master, NULL);
    for (int i = 0; i < NUM_WORKER; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&workers[i], NULL, T_Worker, id);
    }

    pthread_join(master, NULL);
    for (int i = 0; i < NUM_WORKER; i++) {
        pthread_join(workers[i], NULL);
    }

    free(array);
    free(queue.buffer);
    
    return 0;
}
