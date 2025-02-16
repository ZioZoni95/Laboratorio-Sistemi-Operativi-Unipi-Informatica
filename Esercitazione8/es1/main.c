#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <queue.h>

Queue queue;
int K, M, N;

// Funzione eseguita dai produttori
void *produttore(void *arg) {
    int id = *(int*)arg;
    free(arg); // Libera subito la memoria

    int messages = (id < K % M) ? (K / M) + 1 : (K / M); // Distribuisce equamente i messaggi

    for(int i = 0; i < messages; i++) {
        int value = id * 100 + i; // Identificatore del messaggio
        enqueue(&queue, value);
        printf("Produttore %d ha inserito %d\n", id, value);
        fflush(stdout);
    }
    return NULL;
}

// Funzione eseguita dai consumatori
void *consumatore(void *arg) {
    while(1) {
        int value = dequeue(&queue);
        if (value == -1) {
            printf("Consumatore terminato.\n");
            fflush(stdout);
            break;
        }
        printf("Consumatore ha letto %d\n", value);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 4) {
        fprintf(stderr, "Uso: %s <M produttori> <N consumatori> <K messaggi>\n", argv[0]);
        return EXIT_FAILURE;
    }

    M = atoi(argv[1]);
    N = atoi(argv[2]);
    K = atoi(argv[3]);

    queue_init(&queue);
    pthread_t produttori[M], consumatori[N];

    // Creazione dei thread produttori
    for(int i = 0; i < M; i++) {
        int *id = malloc(sizeof(int));
        if (id == NULL) {
            perror("Errore allocazione memoria");
            exit(EXIT_FAILURE);
        }
        *id = i;
        pthread_create(&produttori[i], NULL, produttore, id);
    }

    // Creazione thread consumatori
    for(int i = 0; i < N; i++) {
        pthread_create(&consumatori[i], NULL, consumatore, NULL);
    }

    // Attendi la terminazione dei produttori
    for(int i = 0; i < M; i++) {
        pthread_join(produttori[i], NULL);
    }

    // Segnala ai consumatori di terminare inserendo N messaggi speciali
    for(int i = 0; i < N; i++) {
        enqueue(&queue, -1);
    }

    // Attende la terminazione dei consumatori
    for(int i = 0; i < N; i++) {
        pthread_join(consumatori[i], NULL);
    }

    queue_destroy(&queue);
    printf("Tutti i thread sono terminati con successo.\n");
    fflush(stdout);
    return EXIT_SUCCESS;
}
