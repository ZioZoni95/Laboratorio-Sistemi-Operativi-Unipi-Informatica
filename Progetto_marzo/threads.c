#include "threads.h"
#include <stdlib.h>
#include <stdio.h>

/*
   Definizione delle variabili globali condivise.
   Queste sono visibili in tutti i file che includono threads.h.
*/
Queue Q;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_results = PTHREAD_COND_INITIALIZER;
int production_finished = 0;
int results_count = 0;
int *array;
int N, k, num_workers, capacity;

/*
   Funzione thread_master:
   - Suddivide l'array in gruppi di coppie (almeno 1 coppia e al massimo k coppie).
   - Inserisce ogni gruppo nella coda condivisa (protetta da mutex e variabili condizione).
   - Al termine della produzione, notifica ai worker la fine della produzione.
   - Infine, raccoglie i risultati parziali dai worker e stampa la somma finale.
*/
void* thread_master(void* arg) {
    int i = 0;
    int group_count = 0;
    printf("Master: Inizio produzione gruppi...\n");
    while (i < N) {
        // Alloca e inizializza un nuovo gruppo
        Group *grp = malloc(sizeof(Group));
        if (!grp) { perror("malloc Group"); exit(EXIT_FAILURE); }
        grp->pairs = malloc(k * sizeof(Pair));
        if (!grp->pairs) { perror("malloc pairs"); exit(EXIT_FAILURE); }
        grp->max_pairs = k;
        grp->num_pairs = 0;

        // Costruisce il gruppo aggiungendo coppie dall'array
        while (grp->num_pairs < k && i < N) {
            if (i + 1 < N) {
                grp->pairs[grp->num_pairs].a = array[i];
                grp->pairs[grp->num_pairs].b = array[i + 1];
                grp->num_pairs++;
                i += 2;
            } else {
                // Se l'array ha un numero dispari di elementi, aggiunge una coppia con il secondo elemento 0
                grp->pairs[grp->num_pairs].a = array[i];
                grp->pairs[grp->num_pairs].b = 0;
                grp->num_pairs++;
                i++;
            }
        }

        group_count++;
        // Prepara un oggetto QueueItem con il tipo ITEM_GROUP
        QueueItem *qi = malloc(sizeof(QueueItem));
        if (!qi) { perror("malloc QueueItem"); exit(EXIT_FAILURE); }
        qi->type = ITEM_GROUP;
        qi->data.group = grp;

        // Inserimento del gruppo nella coda: in attesa se la coda è piena
        pthread_mutex_lock(&mutex);
        while (Q.count == Q.capacity) {
            printf("Master: coda piena, attendo su cond_not_full...\n");
            fflush(stdout);
            pthread_cond_wait(&cond_not_full, &mutex);
            printf("Master: svegliato da cond_not_full\n");
            fflush(stdout);
        }
        queue_push(&Q, qi);
        printf("Master: [Gruppo %d] Prodotto con %d coppie: ", group_count, grp->num_pairs);
        for (int j = 0; j < grp->num_pairs; j++) {
            printf("(%d,%d) ", grp->pairs[j].a, grp->pairs[j].b);
        }
        printf("\n");
        fflush(stdout);
        // Notifica ai thread che la coda non è vuota
        pthread_cond_signal(&cond_not_empty);
        printf("Master: segnalato cond_not_empty dopo push\n");
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
    }

    // Notifica la fine della produzione ai worker
    pthread_mutex_lock(&mutex);
    production_finished = 1;
    printf("Master: Fine produzione. Totale gruppi prodotti: %d\n", group_count);
    pthread_cond_broadcast(&cond_not_empty);
    printf("Master: Broadcast cond_not_empty per notificare fine produzione\n");
    fflush(stdout);
    pthread_mutex_unlock(&mutex);

    // Raccolta dei risultati parziali dai worker
    int final_sum = 0;
    pthread_mutex_lock(&mutex);
    while (results_count < num_workers) {
        printf("Master: in attesa dei risultati (results_count = %d)...\n", results_count);
        fflush(stdout);
        pthread_cond_wait(&cond_results, &mutex);
        printf("Master: svegliato da cond_results (results_count = %d)\n", results_count);
        fflush(stdout);
    }

    printf("Master: Inizio raccolta risultati...\n");
    // Preleva dalla coda i risultati inviati dai worker
    for (int j = 0; j < num_workers; j++) {
        QueueItem *qi = queue_pop(&Q);
        if (qi == NULL || qi->type != ITEM_RESULT) {
            fprintf(stderr, "Master: Errore nella raccolta dei risultati\n");
            exit(EXIT_FAILURE);
        }
        final_sum += *(qi->data.result);
        printf("Master: Ricevuto risultato: %d\n", *(qi->data.result));
        free(qi->data.result);
        free(qi);
        pthread_cond_signal(&cond_not_full);
        printf("Master: segnalato cond_not_full dopo raccolta risultato\n");
        fflush(stdout);
    }
    pthread_mutex_unlock(&mutex);

    printf("Master: Somma finale calcolata: %d\n", final_sum);
    fflush(stdout);
    return NULL;
}

/*
   Funzione thread_worker:
   - Il worker preleva dalla coda gruppi (QueueItem di tipo ITEM_GROUP) e ne elabora le coppie,
     accumulando una somma parziale.
   - Se non ci sono più gruppi (e la produzione è finita) o viene trovato un risultato in coda,
     esce dal ciclo.
   - Al termine, prepara un oggetto QueueItem di tipo ITEM_RESULT e lo inserisce nella coda.
*/
void* thread_worker(void* arg) {
    WorkerArgs *wargs = (WorkerArgs *) arg;
    int worker_id = wargs->worker_id;
    int partial_sum = 0;

    printf("Worker %d: Avviato.\n", worker_id);
    fflush(stdout);

    while (1) {
        pthread_mutex_lock(&mutex);
        while (Q.count == 0 && !production_finished) {
            printf("Worker %d: coda vuota, attendo su cond_not_empty...\n", worker_id);
            fflush(stdout);
            pthread_cond_wait(&cond_not_empty, &mutex);
            printf("Worker %d: svegliato da cond_not_empty\n", worker_id);
            fflush(stdout);
        }

        // Se la coda è vuota e la produzione è finita, esce dal ciclo
        if (Q.count == 0 && production_finished) {
            printf("Worker %d: coda vuota e produzione finita, esco...\n", worker_id);
            fflush(stdout);
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Se il prossimo oggetto è un risultato e la produzione è finita,
        // significa che altri worker hanno già terminato.
        if (Q.count > 0) {
            QueueItem *peek = queue_peek(&Q);
            if (peek->type == ITEM_RESULT && production_finished) {
                printf("Worker %d: rilevato risultato in coda e produzione finita, esco...\n", worker_id);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                break;
            }
        }

        // Preleva un oggetto dalla coda
        QueueItem *qi = queue_pop(&Q);
        pthread_cond_signal(&cond_not_full);
        printf("Worker %d: ha prelevato un oggetto dalla coda\n", worker_id);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);

        if (qi->type != ITEM_GROUP) {
            // Se si preleva erroneamente un oggetto non di tipo GROUP,
            // lo reinserisce e termina.
            printf("Worker %d: errore: oggetto non di tipo GROUP, reinserisco e termino\n", worker_id);
            fflush(stdout);
            pthread_mutex_lock(&mutex);
            queue_push(&Q, qi);
            pthread_cond_signal(&cond_not_empty);
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Elabora il gruppo prelevato
        Group *grp = qi->data.group;
        free(qi);  // Libera il QueueItem, il gruppo resta in memoria
        printf("Worker %d: Inizio elaborazione gruppo con %d coppie: ", worker_id, grp->num_pairs);
        for (int i = 0; i < grp->num_pairs; i++) {
            printf("(%d,%d) ", grp->pairs[i].a, grp->pairs[i].b);
        }
        printf("\n");
        fflush(stdout);

        int group_sum = 0;
        for (int i = 0; i < grp->num_pairs; i++) {
            group_sum += grp->pairs[i].a + grp->pairs[i].b;
        }
        partial_sum += group_sum;

        printf("Worker %d: Gruppo elaborato, somma del gruppo = %d, somma parziale = %d\n", worker_id, group_sum, partial_sum);
        fflush(stdout);

        free(grp->pairs);
        free(grp);
    }

    printf("Worker %d: Terminato, somma parziale finale = %d\n", worker_id, partial_sum);
    fflush(stdout);

    // Prepara l'oggetto risultato da inviare al master
    int *result = malloc(sizeof(int));
    if (!result) { perror("malloc result"); exit(EXIT_FAILURE); }
    *result = partial_sum;
    QueueItem *qi_result = malloc(sizeof(QueueItem));
    if (!qi_result) { perror("malloc QueueItem"); exit(EXIT_FAILURE); }
    qi_result->type = ITEM_RESULT;
    qi_result->data.result = result;

    pthread_mutex_lock(&mutex);
    while (Q.count == Q.capacity) {
        printf("Worker %d: coda piena, attendo su cond_not_full prima di pushare risultato\n", worker_id);
        fflush(stdout);
        pthread_cond_wait(&cond_not_full, &mutex);
        printf("Worker %d: svegliato da cond_not_full per pushare risultato\n", worker_id);
        fflush(stdout);
    }
    queue_push(&Q, qi_result);
    results_count++;
    printf("Worker %d: Risultato parziale inviato (%d). Totale risultati ricevuti finora: %d\n", worker_id, *result, results_count);
    fflush(stdout);
    pthread_cond_signal(&cond_results);
    pthread_cond_signal(&cond_not_empty);
    pthread_mutex_unlock(&mutex);

    free(wargs);
    return NULL;
}
