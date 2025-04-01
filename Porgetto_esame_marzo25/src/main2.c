#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define SENTINEL -1  // Segnale di terminazione per i Worker

// Struttura della coda FIFO condivisa
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

// Inizializza la coda
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

// Inserisce un elemento nella coda
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

// Estrae un elemento dalla coda
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
pthread_mutex_t sommaMutex;
int sommaFinale = 0;

// Funzione per la somma parziale
int sommaGruppo(int *gruppo, int size) {
    int somma = 0;
    for (int i = 0; i < size; i++) {
        somma += gruppo[i];
    }
    return somma;
}

// **Master Thread**
void *T_Master(void *arg) {
    printf("[Master] Inizio produzione gruppi...\n");

    // **Suddivisione dell'array in gruppi di massimo k coppie (2k elementi)**
    int i = 0;
    while (i < N) {
        int size = ((i + 2 * k) <= N) ? (2 * k) : (N - i);
        
        printf("[Master] Gruppo (size: %d): ", size);
        for (int j = 0; j < size; j++) {
            printf("%d ", array[i + j]);
            queue_push(&queue, array[i + j]);  // Inserisco nella coda
        }
        printf("\n");
        
        queue_push(&queue, SENTINEL);  // Segnale per il Worker di elaborare il gruppo
        i += size;
    }

    // **Notifico ai Worker che non ci sono più gruppi**
    for (int i = 0; i < NUM_WORKER; i++) {
        queue_push(&queue, SENTINEL);
    }

    printf("[Master] Tutti i gruppi inseriti. Attendo risultati...\n");

    // **Raccolta dei risultati parziali dai Worker**
    for (int i = 0; i < NUM_WORKER; i++) {
        int parziale = queue_pop(&queue);
        printf("[Master] Ricevuta somma parziale: %d\n", parziale);
        sommaFinale += parziale;
    }

    printf("[Master] Somma totale finale: %d\n", sommaFinale);
    pthread_exit(NULL);
}

// **Worker Thread**
void *T_Worker(void *arg) {
    int sommaParziale = 0;

    while (1) {
        int gruppo[2 * k]; // Buffer per il gruppo
        int size = 0; 

        // **Prelievo di un gruppo**
        while (size < 2 * k) {
            int valore = queue_pop(&queue);
            if (valore == SENTINEL) break;
            gruppo[size++] = valore;
        }

        if (size == 0) break;  // Nessun valore significa terminazione

        // **Calcolo della somma del gruppo**
        printf("[Worker %ld] Processando gruppo:", pthread_self());
        int sommaGruppo = 0;
        for (int i = 0; i < size; i++) {
            printf(" %d", gruppo[i]);
            sommaGruppo += gruppo[i];
        }
        printf("\n");

        sommaParziale += sommaGruppo;
        printf("[Worker %ld] Somma gruppo: %d, Somma accumulata: %d\n", 
               pthread_self(), sommaGruppo, sommaParziale);
    }

    // **Inserimento del risultato parziale nella coda SOLO ALLA FINE**
    queue_push(&queue, sommaParziale);
    printf("[Worker %ld] Inserito nella coda il risultato parziale: %d\n", 
           pthread_self(), sommaParziale);

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

    // **Generazione array casuale**
    array = malloc(N * sizeof(int));
    printf("[Main] Array generato: ");
    for (int i = 0; i < N; i++) {
        array[i] = rand() % 10 + 1;
        printf("%d ", array[i]);
    }
    printf("\n");

    // **Inizializzazione coda e mutex**
    queue_init(&queue, C);
    pthread_mutex_init(&sommaMutex, NULL);

    pthread_t master;
    pthread_t workers[NUM_WORKER];

    // **Avvio Master e Worker**
    pthread_create(&master, NULL, T_Master, NULL);
    for (int i = 0; i < NUM_WORKER; i++) {
        pthread_create(&workers[i], NULL, T_Worker, NULL);
    }

    // **Attesa della terminazione**
    pthread_join(master, NULL);
    for (int i = 0; i < NUM_WORKER; i++) {
        pthread_join(workers[i], NULL);
    }

    // **Pulizia memoria**
    free(array);
    free(queue.buffer);
    
    return 0;
}
