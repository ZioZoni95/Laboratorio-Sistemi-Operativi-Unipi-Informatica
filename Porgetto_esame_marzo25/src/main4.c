#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    int* coppie;     
    int num_coppie;  
    int is_partial;  
} gruppo_t;

typedef struct {
    gruppo_t* buffer;
    int capacity;
    int size;
    int in;
    int out;
} coda_t;

pthread_mutex_t mutex;
pthread_cond_t can_produce;
pthread_cond_t can_consume;
coda_t Q;

int N, k, C, worker_count;
int* array;
int finished = 0;
int active_workers = 0;

void coda_init(coda_t* q, int capacity) {
    q->buffer = (gruppo_t*)malloc(capacity * sizeof(gruppo_t));
    q->capacity = capacity;
    q->size = q->in = q->out = 0;
    printf("[CODA] Inizializzata coda con capacità %d\n", capacity);
}

void coda_push(coda_t* q, gruppo_t g) {
    pthread_mutex_lock(&mutex);
    printf("[CODA] Tentativo inserimento (Size: %d/%d)\n", q->size, q->capacity);
    while(q->size == q->capacity) {
        printf("[CODA] Coda piena, attendo...\n");
        pthread_cond_wait(&can_produce, &mutex);
    }
    
    q->buffer[q->in] = g;
    q->in = (q->in + 1) % q->capacity;
    q->size++;
    
    printf("[CODA] Inserito %s (Size: %d) - Coppie: [", 
           g.is_partial ? "PARZIALE" : "GRUPPO", q->size);
    for(int i=0; i<g.num_coppie*2; i++) {
        printf("%d%s", g.coppie[i], (i == g.num_coppie*2-1) ? "" : ", ");
    }
    printf("]\n");
    
    pthread_cond_broadcast(&can_consume);
    pthread_mutex_unlock(&mutex);
}

gruppo_t coda_pop(coda_t* q) {
    pthread_mutex_lock(&mutex);
    printf("[CODA] Tentativo prelievo (Size: %d/%d)\n", q->size, q->capacity);
    while(q->size == 0 && !finished) {
        printf("[CODA] Coda vuota, attendo...\n");
        pthread_cond_wait(&can_consume, &mutex);
    }
    
    gruppo_t g = q->buffer[q->out];
    q->out = (q->out + 1) % q->capacity;
    q->size--;
    
    printf("[CODA] Rimosso %s (Size: %d) - Coppie: [", 
           g.is_partial ? "PARZIALE" : "GRUPPO", q->size);
    for(int i=0; i<g.num_coppie*2; i++) {
        printf("%d%s", g.coppie[i], (i == g.num_coppie*2-1) ? "" : ", ");
    }
    printf("]\n");
    
    pthread_cond_signal(&can_produce);
    pthread_mutex_unlock(&mutex);
    return g;
}

// Thread Master
void* T_Master(void* arg) {
    int index = 0;
    printf("[MASTER] Avvio produzione (N=%d, k=%d)\n", N, k);
    
    while(index < N) {
        gruppo_t gruppo;
        gruppo.num_coppie = 0;
        gruppo.coppie = (int*)malloc(2*k * sizeof(int));
        gruppo.is_partial = 0;

        // Riempie il gruppo con k coppie (o fino alla fine dell'array)
        for(int i=0; i<k && index<N; i++) {
            int pos1 = index++;
            int pos2 = (index < N) ? index++ : pos1;
            
            gruppo.coppie[2*i] = array[pos1];
            gruppo.coppie[2*i+1] = array[pos2];
            gruppo.num_coppie++;
            
            printf("[MASTER] Aggiunta coppia (%d, %d)\n", array[pos1], array[pos2]);
        }

        coda_push(&Q, gruppo);
        printf("[MASTER] Inserito gruppo %d con %d coppie\n", 
               (index/(2*k))+1, gruppo.num_coppie);
    }

    printf("[MASTER] Fine produzione\n");
    pthread_mutex_lock(&mutex);
    finished = 1;
    active_workers = worker_count;
    pthread_cond_broadcast(&can_consume); // Sveglia tutti i worker bloccati
    pthread_mutex_unlock(&mutex);

    // Raccoglie risultati
    int total = 0;
    while(active_workers > 0) {
        pthread_mutex_lock(&mutex);
        printf("[MASTER] In attesa di risultati (Rimanenti: %d)\n", active_workers);
        
        while(Q.size == 0 && active_workers > 0) {
            pthread_cond_wait(&can_consume, &mutex);
        }
        
        if(Q.size > 0) {
            gruppo_t res = coda_pop(&Q);
            if(res.is_partial) {
                total += res.coppie[0];
                active_workers--;
                printf("[MASTER] Ricevuto parziale: %d (Totale corrente: %d)\n", res.coppie[0], total);
                free(res.coppie);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    printf("[MASTER] Somma finale: %d\n", total);
    return NULL;
}

// Thread Worker
void* T_Worker(void* arg) {
    int partial_sum = 0;
    printf("[WORKER] Avvio\n");
    
    while(1) {
        printf("[WORKER] Tentativo prelievo gruppo\n");
        gruppo_t gruppo = coda_pop(&Q);
        
        if(gruppo.is_partial) {
            printf("[WORKER] Trovato risultato parziale, reinserisco\n");
            coda_push(&Q, gruppo);
            continue;
        }
        
        // Elaborazione gruppo
        int sum = 0;
        for(int i=0; i<gruppo.num_coppie*2; i++) {
            sum += gruppo.coppie[i];
        }
        partial_sum += sum;
        printf("[WORKER] Elaborato gruppo: somma %d (Parziale: %d)\n", sum, partial_sum);
        free(gruppo.coppie);

        // Controlla se deve terminare
        pthread_mutex_lock(&mutex);
        if(finished && Q.size == 0) {
            gruppo_t res;
            res.coppie = (int*)malloc(sizeof(int));
            res.coppie[0] = partial_sum;
            res.is_partial = 1;
            res.num_coppie = 1;
            
            printf("[WORKER] Inserisco risultato parziale: %d\n", partial_sum);
            coda_push(&Q, res);
            
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
    }
    
    printf("[WORKER] Terminazione\n");
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 5) {
        printf("Usage: %s worker_count N k C\n", argv[0]);
        return 1;
    }

    worker_count = atoi(argv[1]);
    N = atoi(argv[2]);
    k = atoi(argv[3]);
    C = atoi(argv[4]);

    array = (int*)malloc(N * sizeof(int));
    for(int i=0; i<N; i++) array[i] = i+1;

    coda_init(&Q, C);
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&can_produce, NULL);
    pthread_cond_init(&can_consume, NULL);

    pthread_t master;
    pthread_t workers[worker_count];

    pthread_create(&master, NULL, T_Master, NULL);
    for(int i=0; i<worker_count; i++) {
        pthread_create(&workers[i], NULL, T_Worker, NULL);
    }

    pthread_join(master, NULL);
    for(int i=0; i<worker_count; i++) {
        pthread_join(workers[i], NULL);
    }

    free(array);
    free(Q.buffer);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);

    return 0;
}