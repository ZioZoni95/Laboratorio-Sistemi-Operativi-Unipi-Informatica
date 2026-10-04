/**
 * @file worker.c
 * @brief Funzione eseguita dai thread Worker per l'ordinamento parallelo.
 *
 * Fasi (come da traccia):
 *  1. (solo Worker 0) calcolo delle partizioni e push dei task (start,end) nella coda, poi chiusura;
 *  2. (tutti) pop di un task alla volta e qsort della partizione, finché la coda è vuota e chiusa;
 *     barriera: tutte le partizioni sono ordinate;
 *  3. (tutti) merge in ceil(log2 Q) passi sincroni: al passo k lavorano solo i Worker che hanno
 *     una coppia di blocchi da fondere, gli altri attendono in barriera.
 *
 * Il merge alterna due buffer ("ping-pong"): ogni passo legge da `src` e scrive in `dst`, poi i
 * ruoli si scambiano. Non serve ricopiare il risultato a ogni passo né proteggere nulla con mutex:
 * ogni Worker scrive un intervallo di `dst` disgiunto da quello degli altri e la barriera a fine
 * passo garantisce che le scritture siano visibili prima del passo successivo.
 * Se i passi sono dispari il risultato finale resta in `temp_array`: se ne occupa il main.
 */

#include "worker.h"
#include "queue.h"
#include "myutils.h"

static long min_long(long a, long b) { return a < b ? a : b; }

// Attende gli altri Worker alla barriera condivisa.
static void barrier_sync(const ThreadArgs *a) {
    int err = pthread_barrier_wait(a->barrier);
    // Esattamente un thread riceve PTHREAD_BARRIER_SERIAL_THREAD: non è un errore.
    if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
        CHECK_PTHREAD_ERR(err, "pthread_barrier_wait");
    }
}

// Fase 1 (solo Worker 0): una partizione per task, saltando quelle vuote (N < Q).
static void phase_partition(const ThreadArgs *a) {
    long pushed = 0;
    for (long i = 0; i < a->n_partitions; ++i) {
        long start = partition_bound(i, a->n_elements, a->n_partitions);
        long end = partition_bound(i + 1, a->n_elements, a->n_partitions) - 1; // estremo incluso
        if (start > end) continue; // partizione vuota
        Partition_Index_Task task = { start, end };
        push(a->queue, task);
        pushed++;
    }
    close_queue(a->queue); // nessun altro task: i consumatori possono terminare
    LOG(a->thread_id, "inseriti %ld task, coda chiusa", pushed);
}

// Fase 2: ordina le partizioni prelevate dalla coda.
static void phase_sort(const ThreadArgs *a) {
    Partition_Index_Task task;
    long sorted = 0;
    while (pop(a->queue, &task)) {
        long count = task.end - task.start + 1;
        LOG(a->thread_id, "qsort partizione [%ld..%ld] (%ld elementi)", task.start, task.end, count);
        qsort(&a->array[task.start], (size_t)count, sizeof(int), qsort_compare);
        sorted++;
    }
    LOG(a->thread_id, "ordinate %ld partizioni", sorted);
}

// Fase 3: merge a passi sincroni con due buffer alternati.
static void phase_merge(const ThreadArgs *a) {
    const long Q = a->n_partitions;
    const int steps = merge_steps(Q);
    int *src = a->array;
    int *dst = a->temp_array;
    long span = 1; // numero di partizioni originali in ciascun blocco di ingresso

    for (int k = 0; k < steps; ++k, span *= 2) {
        long blocks = (Q + span - 1) / span; // blocchi di ingresso a questo passo
        long tasks = (blocks + 1) / 2;       // coppie da fondere (l'ultimo blocco può restare spaiato)

        // Il Worker t fonde la coppia t, t+P, t+2P, ...: con Q = P lavorano i primi P/2^(k+1).
        for (long t = a->thread_id; t < tasks; t += a->n_threads) {
            long first = 2 * t * span;                 // prima partizione del blocco sinistro
            long middle = min_long(first + span, Q);   // prima partizione del blocco destro
            long last = min_long(first + 2 * span, Q); // una oltre l'ultima partizione della coppia

            long lo = partition_bound(first, a->n_elements, Q);
            long mid = partition_bound(middle, a->n_elements, Q);
            long hi = partition_bound(last, a->n_elements, Q);

            LOG(a->thread_id, "passo %d: fondo [%ld..%ld) + [%ld..%ld)", k, lo, mid, mid, hi);
            merge_sections(src, dst, lo, mid, hi); // blocco spaiato: mid == hi, semplice copia
        }

        barrier_sync(a); // fine passo: tutte le scritture su dst sono visibili
        int *tmp = src; // scambio dei ruoli dei due buffer
        src = dst;
        dst = tmp;
    }
}

void *worker_thread(void *args) {
    const ThreadArgs *a = (const ThreadArgs *)args;

    if (a->thread_id == 0) phase_partition(a);

    phase_sort(a);
    barrier_sync(a); // tutte le partizioni sono ordinate

    phase_merge(a);

    LOG(a->thread_id, "terminato");
    return NULL;
}
