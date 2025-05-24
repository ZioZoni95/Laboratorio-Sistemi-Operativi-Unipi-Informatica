#define _GNU_SOURCE  // Questa direttiva DEVE essere definita prima di includere <pthread.h>
#include "threads.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


/* Definizione delle variabili globali condivise.
   Queste variabili sono accessibili anche dagli altri file che includono "threads.h" tramite extern.
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

/* Variabile globale per la modalità debug:
   debug_mode == 1 -> output dettagliato, debug extra.
   debug_mode == 0 -> output standard.
*/
int debug_mode = 0;

/*
 * Funzione print_stack_info:
 * Stampa informazioni sullo stack del thread corrente (base e dimensione),
 * utilizzando la funzione non standard pthread_getattr_np (disponibile su Linux).
 */
void print_stack_info(const char *thread_name, int id) {
    pthread_attr_t attr;
    int ret = pthread_getattr_np(pthread_self(), &attr);
    if(ret != 0) {
        perror("pthread_getattr_np");
        return;
    }
    
    void *stack_addr;
    size_t stack_size;
    pthread_attr_getstack(&attr, &stack_addr, &stack_size);
    printf("%s %d: [DEBUG] Stack base address = %p, Stack size = %zu bytes\n", thread_name, id, stack_addr, stack_size);
    fflush(stdout);
    pthread_attr_destroy(&attr);
}

/*
 * Funzione thread_master:
 *
 * 1. Suddivide l'array in gruppi di coppie:
 *    - Ogni gruppo contiene almeno 1 e al massimo k coppie.
 * 2. Inserisce ogni gruppo (converso in un QueueItem di tipo ITEM_GROUP) nella coda Q.
 * 3. Al termine della produzione, imposta production_finished = 1 e notifica tutti i Worker.
 * 4. Raccoglie i QueueItem di tipo ITEM_RESULT dalla coda, sommando i risultati parziali per ottenere
 *    la somma finale.
 *
 * Vengono stampate informazioni di debug (se debug_mode è attivo) sugli indirizzi e sullo stack.
 */
void* thread_master(void* arg) {
    // Informazioni sullo stack (debug)
    if (debug_mode)
        print_stack_info("Master", 0);
    
    int i = 0;
    int group_count = 0;
    printf("Master: Inizio produzione gruppi...\n");
    fflush(stdout);
    
    // Produzione dei gruppi (lettura dall'array e raggruppamento in coppie)
    while (i < N) {
        Group *grp = malloc(sizeof(Group));
        if (!grp) { perror("malloc Group"); exit(EXIT_FAILURE); }
        grp->pairs = malloc(k * sizeof(Pair));
        if (!grp->pairs) { perror("malloc pairs"); exit(EXIT_FAILURE); }
        grp->max_pairs = k;
        grp->num_pairs = 0;
        
        while (grp->num_pairs < k && i < N) {
            if (i + 1 < N) {
                grp->pairs[grp->num_pairs].a = array[i];
                grp->pairs[grp->num_pairs].b = array[i + 1];
                grp->num_pairs++;
                i += 2;
            } else {
                grp->pairs[grp->num_pairs].a = array[i];
                grp->pairs[grp->num_pairs].b = 0;
                grp->num_pairs++;
                i++;
            }
        }
        group_count++;
        
        // Prepara il QueueItem con il gruppo
        QueueItem *qi = malloc(sizeof(QueueItem));
        if (!qi) { perror("malloc QueueItem"); exit(EXIT_FAILURE); }
        qi->type = ITEM_GROUP;
        qi->data.group = grp;
        
        pthread_mutex_lock(&mutex);
        while (Q.count == Q.capacity) {
            if (debug_mode) {
                printf("Master: Coda piena, attendo su cond_not_full...\n");
                fflush(stdout);
            }
            pthread_cond_wait(&cond_not_full, &mutex);
            if (debug_mode) {
                printf("Master: Svegliato da cond_not_full\n");
                fflush(stdout);
            }
        }
        queue_push(&Q, qi);
        printf("DEBUG: [Coda] count = %d, head = %d, tail = %d\n", Q.count, Q.head, Q.tail);
        printf("Master: [Gruppo %d] Prodotto con %d coppie: ", group_count, grp->num_pairs);
        for (int j = 0; j < grp->num_pairs; j++) {
            printf("(%d,%d) ", grp->pairs[j].a, grp->pairs[j].b);
        }
        printf("\n");
        fflush(stdout);
        pthread_cond_signal(&cond_not_empty);
        if (debug_mode) {
            printf("Master: Segnalato cond_not_empty dopo push\n");
            fflush(stdout);
        }
        pthread_mutex_unlock(&mutex);
    }
    
    // Fine produzione: notifica a tutti i Worker
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
    while (collected < num_workers) {
        while (Q.count == 0) {
            pthread_cond_wait(&cond_not_empty, &mutex);
        }
        QueueItem *qi = queue_peek(&Q);
        if (qi->type == ITEM_RESULT) {
            qi = queue_pop(&Q);
            printf("DEBUG: [Coda] count = %d, head = %d, tail = %d\n", Q.count, Q.head, Q.tail);
            final_sum += *(qi->data.result);
            printf("Master: Ricevuto risultato: %d\n", *(qi->data.result));
            free(qi->data.result);
            free(qi);
            collected++;
            pthread_cond_signal(&cond_not_full);
        } else {
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
 *
 * 1. Ogni Worker attende finché la coda è vuota e production_finished è falso.
 * 2. Se la coda contiene un QueueItem di tipo ITEM_GROUP, lo preleva ed elabora il gruppo
 *    (calcolando la somma delle coppie) e aggiornando la somma parziale.
 * 3. Se la coda è vuota e production_finished è vero, oppure se il peek mostra un ITEM_RESULT,
 *    il Worker esce dal ciclo.
 * 4. Infine, il Worker prepara un QueueItem di tipo ITEM_RESULT con il risultato parziale e lo inserisce nella coda,
 *    aggiornando results_count e notificando il Master tramite cond_results.
 *
 * Vengono stampati messaggi di debug, incluso l'indirizzo del parametro e le informazioni sullo stack.
 */
void* thread_worker(void* arg) {
    WorkerArgs *wargs = (WorkerArgs *) arg;
    int worker_id = wargs->worker_id;
    
    // Debug: stampiamo informazioni sullo stack e sull'indirizzo del parametro
    if (debug_mode) {
        print_stack_info("Worker", worker_id);
        printf("Worker %d: Indirizzo di wargs = %p\n", worker_id, (void*)wargs);
        fflush(stdout);
        // Stampiamo l'ID pthread e l'indirizzo di wargs
        printf("Worker %d (pthread id %lu): Avviato. Indirizzo di wargs: %p\n", worker_id, pthread_self(), (void*)wargs);
        fflush(stdout);

        // Stampiamo anche un dummy per vedere l'indirizzo dello stack
        int dummy;
        printf("Worker %d: indirizzo variabile dummy = %p\n", worker_id, (void*)&dummy);
        fflush(stdout);
    }

    
    
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
        
        if (Q.count == 0 && production_finished) {
            printf("Worker %d: Coda vuota e produzione finita, esco...\n", worker_id);
            fflush(stdout);
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        if (Q.count > 0) {
            QueueItem *peek = queue_peek(&Q);
            if (peek->type == ITEM_RESULT && production_finished) {
                printf("Worker %d: Rilevato risultato in coda e produzione finita, esco...\n", worker_id);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        
        QueueItem *qi = queue_pop(&Q);
        printf("DEBUG: [Coda] count = %d, head = %d, tail = %d\n", Q.count, Q.head, Q.tail);
        pthread_cond_signal(&cond_not_full);
        printf("Worker %d: Ha prelevato un oggetto dalla coda\n", worker_id);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
        
        if (qi->type != ITEM_GROUP) {
            printf("Worker %d: Errore: oggetto non di tipo GROUP, reinserisco e termino\n", worker_id);
            fflush(stdout);
            pthread_mutex_lock(&mutex);
            queue_push(&Q, qi);
            printf("DEBUG: [Coda] count = %d, head = %d, tail = %d\n", Q.count, Q.head, Q.tail);
            pthread_cond_signal(&cond_not_empty);
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        Group *grp = qi->data.group;
        free(qi);
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
    
    int *result = malloc(sizeof(int));
    if (!result) { perror("malloc result"); exit(EXIT_FAILURE); }
    *result = partial_sum;
    QueueItem *qi_result = malloc(sizeof(QueueItem));
    if (!qi_result) { perror("malloc QueueItem"); exit(EXIT_FAILURE); }
    qi_result->type = ITEM_RESULT;
    qi_result->data.result = result;
    
    pthread_mutex_lock(&mutex);
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
    pthread_cond_signal(&cond_results);
    pthread_cond_signal(&cond_not_empty);
    pthread_mutex_unlock(&mutex);
    
    free(wargs);
    return NULL;
}
