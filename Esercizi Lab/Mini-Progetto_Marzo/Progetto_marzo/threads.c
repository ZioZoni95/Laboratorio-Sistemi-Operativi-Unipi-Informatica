#include "threads.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * Definizione delle variabili globali condivise.
 * Queste sono accessibili anche agli altri file che includono "threads.h" tramite la parola chiave extern.
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
 * Funzione thread_master:
 * 1. Suddivide l'array in gruppi di coppie:
 *      - Il Master legge dall'array e raggruppa gli elementi in gruppi; ogni gruppo contiene
 *        almeno una coppia e al massimo k coppie.
 * 2. Inserisce ogni gruppo come un QueueItem di tipo ITEM_GROUP nella coda Q.
 * 3. Al termine della produzione dei gruppi, il Master imposta production_finished = 1 e notifica
 *    tutti i Worker tramite un broadcast.
 * 4. Dopo la produzione, il Master raccoglie i risultati inviati dai Worker (QueueItem di tipo ITEM_RESULT)
 *    estraendoli appena disponibili (utile anche quando la capacità della coda è 1) e calcola la somma finale.
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
                // Se l'array ha un numero dispari di elementi, aggiunge il rimanente con il secondo elemento = 0
                grp->pairs[grp->num_pairs].a = array[i];
                grp->pairs[grp->num_pairs].b = 0;
                grp->num_pairs++;
                i++;
            }
        }
        group_count++;
        
        // Prepara un QueueItem per il gruppo
        QueueItem *qi = malloc(sizeof(QueueItem));
        if (!qi) { perror("malloc QueueItem"); exit(EXIT_FAILURE); }
        qi->type = ITEM_GROUP;
        qi->data.group = grp;
        
        pthread_mutex_lock(&mutex);
        // Attende se la coda è piena
        while (Q.count == Q.capacity) {
            printf("Master: Coda piena, attendo su cond_not_full...\n");
            fflush(stdout);
            pthread_cond_wait(&cond_not_full, &mutex);
            printf("Master: Svegliato da cond_not_full\n");
            fflush(stdout);
        }
        queue_push(&Q, qi);
        // Stampa dettagli sul gruppo prodotto (includendo le coppie)
        printf("Master: [Gruppo %d] Prodotto con %d coppie: ", group_count, grp->num_pairs);
        for (int j = 0; j < grp->num_pairs; j++) {
            printf("(%d,%d) ", grp->pairs[j].a, grp->pairs[j].b);
        }
        printf("\n");
        fflush(stdout);
        // Notifica a chi attende che la coda non è più vuota
        pthread_cond_signal(&cond_not_empty);
        printf("Master: Segnalato cond_not_empty dopo push\n");
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
    }
    
    // Fine produzione: il Master imposta la flag e notifica a tutti i Worker
    pthread_mutex_lock(&mutex);
    production_finished = 1;
    printf("Master: Fine produzione. Totale gruppi prodotti: %d\n", group_count);
    pthread_cond_broadcast(&cond_not_empty);
    printf("Master: Broadcast cond_not_empty per notificare fine produzione\n");
    fflush(stdout);
    pthread_mutex_unlock(&mutex);
    
    // Raccolta dei risultati parziali dai Worker
    int collected = 0;
    int final_sum = 0;
    pthread_mutex_lock(&mutex);
    printf("Master: Inizio raccolta risultati...\n");
    // Finché non sono stati raccolti i risultati di tutti i Worker, attende e preleva quelli disponibili
    while (collected < num_workers) {
        while (Q.count == 0) {
            pthread_cond_wait(&cond_not_empty, &mutex);
        }
        // Usa queue_peek per controllare se il prossimo elemento è un risultato
        QueueItem *qi = queue_peek(&Q);
        if (qi->type == ITEM_RESULT) {
            qi = queue_pop(&Q);
            final_sum += *(qi->data.result);
            printf("Master: Ricevuto risultato: %d\n", *(qi->data.result));
            free(qi->data.result);
            free(qi);
            collected++;
            pthread_cond_signal(&cond_not_full);
        } else {
            // Se il prossimo elemento non è un risultato, attende ulteriormente
            pthread_cond_wait(&cond_not_empty, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    
    printf("Master: Somma finale calcolata: %d\n", final_sum);
    fflush(stdout);
    return NULL;
}

/*
 * Funzione thread_worker:
 * 1. Ogni Worker attende finché la coda è vuota e la produzione non è finita, usando cond_not_empty.
 * 2. Quando la coda contiene un elemento (QueueItem di tipo ITEM_GROUP), il Worker lo preleva e lo elabora:
 *      - Il Worker somma tutte le coppie contenute nel gruppo e aggiorna la propria somma parziale.
 * 3. Se la coda è vuota e production_finished è vero (o se il peek mostra un ITEM_RESULT), il Worker esce dal ciclo.
 * 4. Al termine, prepara un QueueItem di tipo ITEM_RESULT contenente il risultato parziale, lo inserisce nella coda
 *    (attendendo se la coda è piena) e segnala il Master.
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
            printf("Worker %d: Coda vuota, attendo su cond_not_empty...\n", worker_id);
            fflush(stdout);
            pthread_cond_wait(&cond_not_empty, &mutex);
            printf("Worker %d: Svegliato da cond_not_empty\n", worker_id);
            fflush(stdout);
        }
        
        // Se la coda è vuota e la produzione è finita, il Worker esce dal ciclo
        if (Q.count == 0 && production_finished) {
            printf("Worker %d: Coda vuota e produzione finita, esco...\n", worker_id);
            fflush(stdout);
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        // Se il prossimo elemento è un risultato e production_finished è attivo, esce
        if (Q.count > 0) {
            QueueItem *peek = queue_peek(&Q);
            if (peek->type == ITEM_RESULT && production_finished) {
                printf("Worker %d: Rilevato risultato in coda e produzione finita, esco...\n", worker_id);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        
        // Preleva un QueueItem dalla coda
        QueueItem *qi = queue_pop(&Q);
        pthread_cond_signal(&cond_not_full);
        printf("Worker %d: Ha prelevato un oggetto dalla coda\n", worker_id);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
        
        if (qi->type != ITEM_GROUP) {
            // Se per errore si preleva un elemento non di tipo ITEM_GROUP, lo reinserisce e termina
            printf("Worker %d: Errore: oggetto non di tipo GROUP, reinserisco e termino\n", worker_id);
            fflush(stdout);
            pthread_mutex_lock(&mutex);
            queue_push(&Q, qi);
            pthread_cond_signal(&cond_not_empty);
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        // Processa il gruppo: il Worker elabora l'oggetto Group
        Group *grp = qi->data.group;
        free(qi); // Libera il QueueItem, il gruppo rimane in memoria
        printf("Worker %d: Inizio elaborazione gruppo con %d coppie: ", worker_id, grp->num_pairs);
        for (int i = 0; i < grp->num_pairs; i++) {
            printf("(%d,%d) ", grp->pairs[i].a, grp->pairs[i].b);
        }
        printf("\n");
        fflush(stdout);
        
        // Calcola la somma del gruppo
        int group_sum = 0;
        for (int i = 0; i < grp->num_pairs; i++) {
            group_sum += grp->pairs[i].a + grp->pairs[i].b;
        }
        partial_sum += group_sum;
        printf("Worker %d: Gruppo elaborato, somma del gruppo = %d, somma parziale = %d\n", worker_id, group_sum, partial_sum);
        fflush(stdout);
        
        // Libera la memoria usata per il gruppo
        free(grp->pairs);
        free(grp);
    }
    
    printf("Worker %d: Terminato, somma parziale finale = %d\n", worker_id, partial_sum);
    fflush(stdout);
    
    // Prepara il risultato parziale in un QueueItem di tipo ITEM_RESULT
    int *result = malloc(sizeof(int));
    if (!result) { perror("malloc result"); exit(EXIT_FAILURE); }
    *result = partial_sum;
    QueueItem *qi_result = malloc(sizeof(QueueItem));
    if (!qi_result) { perror("malloc QueueItem"); exit(EXIT_FAILURE); }
    qi_result->type = ITEM_RESULT;
    qi_result->data.result = result;
    
    pthread_mutex_lock(&mutex);
    // Attende se la coda è piena prima di pushare il risultato
    while (Q.count == Q.capacity) {
        printf("Worker %d: Coda piena, attendo su cond_not_full prima di pushare risultato\n", worker_id);
        fflush(stdout);
        pthread_cond_wait(&cond_not_full, &mutex);
        printf("Worker %d: Svegliato da cond_not_full per pushare risultato\n", worker_id);
        fflush(stdout);
    }
    queue_push(&Q, qi_result);
    results_count++;
    printf("Worker %d: Risultato parziale inviato (%d). Totale risultati finora: %d\n", worker_id, *result, results_count);
    fflush(stdout);
    pthread_cond_signal(&cond_results);  // Notifica al Master l'arrivo di un risultato
    pthread_cond_signal(&cond_not_empty);
    pthread_mutex_unlock(&mutex);
    
    free(wargs);
    return NULL;
}
