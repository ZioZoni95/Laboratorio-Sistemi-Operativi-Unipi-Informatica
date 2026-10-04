/**
 * @file main.c
 * @brief Ordinamento parallelo di un array di interi con P Worker e una coda concorrente.
 *
 * Il main legge le opzioni, genera un array casuale, avvia i Worker, ne attende la terminazione
 * e verifica il risultato. L'exit code è 0 solo se l'array finale è ordinato ed è una permutazione
 * dell'array iniziale.
 */

#include <limits.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "queue.h"
#include "worker.h"
#include "myutils.h"

#define MAX_N          INT_MAX     // elementi
#define MAX_WORKERS    4096        // thread
#define MAX_PARTITIONS (1L << 20)  // partizioni

int g_verbose = 0;

static void usage(const char *prog, FILE *out) {
    fprintf(out,
        "Uso: %s -n <elementi> -w <worker> [-p <partizioni>] [-s <seme>] [-c] [-v] [-h]\n"
        "  -n N   numero di elementi dell'array, 1..%d (obbligatorio)\n"
        "  -w P   numero di thread Worker, 1..%d (obbligatorio)\n"
        "  -p Q   numero di partizioni iniziali, 1..%ld (default: P)\n"
        "  -s S   seme del generatore casuale (default: ora corrente)\n"
        "  -c     confronta anche il risultato con un qsort sequenziale (verifica esatta)\n"
        "  -v     stampa le partizioni ordinate e i merge eseguiti da ciascun Worker\n"
        "  -h     mostra questo messaggio\n",
        prog, MAX_N, MAX_WORKERS, MAX_PARTITIONS);
}

// Converte `str` in un intero in [min, max]; termina il programma se non è valido.
static long parse_long(const char *str, char opt, long min, long max) {
    char *end = NULL;
    errno = 0;
    long value = strtol(str, &end, 10);
    if (errno != 0 || end == str || *end != '\0' || value < min || value > max) {
        fprintf(stderr, "Errore: valore non valido per -%c: '%s' (atteso un intero in [%ld, %ld]).\n",
                opt, str, min, max);
        exit(EXIT_FAILURE);
    }
    return value;
}

// Somma e XOR di tutti gli elementi: identificano con alta probabilità la permutazione.
typedef struct { unsigned long long sum; unsigned xr; } Checksum;

static Checksum checksum(const int *a, long n) {
    Checksum c = { 0, 0 };
    for (long i = 0; i < n; ++i) {
        c.sum += (unsigned long long)(unsigned)a[i];
        c.xr ^= (unsigned)a[i];
    }
    return c;
}

static double elapsed_seconds(const struct timespec *t0, const struct timespec *t1) {
    return (double)(t1->tv_sec - t0->tv_sec) + (double)(t1->tv_nsec - t0->tv_nsec) * 1e-9;
}

int main(int argc, char *argv[]) {
    long n = 0, parts = 0;
    int p = 0, check_reference = 0;
    unsigned seed = (unsigned)time(NULL);

    // --- Opzioni ---
    int opt;
    while ((opt = getopt(argc, argv, "n:w:p:s:cvh")) != -1) {
        switch (opt) {
            case 'n': n = parse_long(optarg, 'n', 1, MAX_N); break;
            case 'w': p = (int)parse_long(optarg, 'w', 1, MAX_WORKERS); break;
            case 'p': parts = parse_long(optarg, 'p', 1, MAX_PARTITIONS); break;
            case 's': seed = (unsigned)parse_long(optarg, 's', 0, (long)UINT_MAX); break;
            case 'c': check_reference = 1; break;
            case 'v': g_verbose = 1; break;
            case 'h': usage(argv[0], stdout); return EXIT_SUCCESS;
            default:  usage(argv[0], stderr); return EXIT_FAILURE;
        }
    }
    if (optind != argc || n == 0 || p == 0) {
        fprintf(stderr, "Errore: -n e -w sono obbligatori e non sono ammessi argomenti extra.\n");
        usage(argv[0], stderr);
        return EXIT_FAILURE;
    }
    if (parts == 0) parts = p; // come da traccia: tante partizioni quanti Worker

    printf("Avvio parallel_sort: N=%ld elementi, P=%d worker, Q=%ld partizioni (seme %u).\n",
           n, p, parts, seed);

    // --- Allocazione e generazione dei dati ---
    int *array = malloc((size_t)n * sizeof(int));
    int *temp_array = malloc((size_t)n * sizeof(int));
    CHECK_ERR(array == NULL || temp_array == NULL, "Errore allocazione array");

    srand(seed);
    long range = (n * 10 > RAND_MAX) ? RAND_MAX : n * 10; // rand() non supera RAND_MAX
    for (long i = 0; i < n; ++i) array[i] = rand() % range;

    Checksum before = checksum(array, n);
    int *reference = NULL;
    if (check_reference) {
        reference = malloc((size_t)n * sizeof(int));
        CHECK_ERR(reference == NULL, "Errore allocazione array di riferimento");
        memcpy(reference, array, (size_t)n * sizeof(int));
        qsort(reference, (size_t)n, sizeof(int), qsort_compare);
    }

    print_array("Array iniziale", array, n);

    // --- Risorse di sincronizzazione e argomenti dei Worker ---
    ConcurrentQueue queue;
    CHECK_ERR(init_queue(&queue) != 0, "Errore inizializzazione coda");

    pthread_barrier_t barrier;
    CHECK_PTHREAD_ERR(pthread_barrier_init(&barrier, NULL, (unsigned)p), "pthread_barrier_init");

    ThreadArgs *args = malloc((size_t)p * sizeof(ThreadArgs));
    pthread_t *threads = malloc((size_t)p * sizeof(pthread_t));
    CHECK_ERR(args == NULL || threads == NULL, "Errore allocazione thread");

    // --- Avvio e attesa dei Worker ---
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < p; ++i) {
        args[i] = (ThreadArgs){
            .thread_id = i, .n_threads = p, .n_partitions = parts, .n_elements = n,
            .array = array, .temp_array = temp_array, .queue = &queue, .barrier = &barrier,
        };
        CHECK_PTHREAD_ERR(pthread_create(&threads[i], NULL, worker_thread, &args[i]), "pthread_create");
    }
    for (int i = 0; i < p; ++i) {
        CHECK_PTHREAD_ERR(pthread_join(threads[i], NULL), "pthread_join");
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("Ordinamento completato in %.3f s.\n", elapsed_seconds(&t0, &t1));

    // Con un numero dispari di passi di merge il risultato è rimasto in temp_array.
    if (merge_steps(parts) % 2 == 1) memcpy(array, temp_array, (size_t)n * sizeof(int));

    print_array("Array finale", array, n);

    // --- Verifica: ordinamento + permutazione (+ confronto con qsort se richiesto) ---
    int ok = 1;
    for (long i = 0; i + 1 < n; ++i) {
        if (array[i] > array[i + 1]) {
            fprintf(stderr, "ERRORE: array non ordinato: array[%ld]=%d > array[%ld]=%d\n",
                    i, array[i], i + 1, array[i + 1]);
            ok = 0;
            break;
        }
    }
    Checksum after = checksum(array, n);
    if (after.sum != before.sum || after.xr != before.xr) {
        fprintf(stderr, "ERRORE: gli elementi finali non sono una permutazione di quelli iniziali.\n");
        ok = 0;
    }
    if (reference != NULL && memcmp(reference, array, (size_t)n * sizeof(int)) != 0) {
        fprintf(stderr, "ERRORE: il risultato differisce dal qsort sequenziale di riferimento.\n");
        ok = 0;
    }
    printf(ok ? "Verifica: L'array è ordinato correttamente.\n" : "Verifica: ERRORE.\n");

    // --- Pulizia ---
    destroy_queue(&queue);
    pthread_barrier_destroy(&barrier);
    free(array);
    free(temp_array);
    free(reference);
    free(args);
    free(threads);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
