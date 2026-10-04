/**
 * @file myutils.c
 * @brief Funzioni di utilità: confronto per qsort, geometria delle partizioni, merge, stampa.
 */

#include <assert.h>
#include "myutils.h"

int qsort_compare(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    // Si confronta invece di sottrarre (x - y può traboccare). La forma con gli `if` è
    // misurabilmente più veloce con qsort della versione senza salti (x > y) - (x < y).
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

long partition_bound(long i, long n, long parts) {
    assert(parts > 0 && i >= 0 && i <= parts);
    long chunk = n / parts;
    long remainder = n % parts;
    return i * chunk + (i < remainder ? i : remainder);
}

int merge_steps(long parts) {
    int steps = 0;
    for (long span = 1; span < parts; span *= 2) steps++;
    return steps;
}

void merge_sections(const int *src, int *dst, long lo, long mid, long hi) {
    assert(src != dst);
    assert(0 <= lo && lo <= mid && mid <= hi);

    long i = lo;  // scorre src[lo..mid)
    long j = mid; // scorre src[mid..hi)
    long k = lo;  // scrive dst[lo..hi)

    while (i < mid && j < hi) {
        dst[k++] = (src[i] <= src[j]) ? src[i++] : src[j++]; // <= mantiene la stabilità
    }
    if (i < mid) memcpy(&dst[k], &src[i], (size_t)(mid - i) * sizeof(int));
    if (j < hi)  memcpy(&dst[k], &src[j], (size_t)(hi - j) * sizeof(int));
}

void print_array(const char *label, const int *arr, long n) {
    const long HEAD = 15; // elementi mostrati all'inizio se l'array è lungo
    const long TAIL = 15; // elementi mostrati alla fine

    printf("--- %s (N=%ld) ---\n[", label, n);
    if (n <= HEAD + TAIL) {
        for (long i = 0; i < n; ++i) printf("%s%d", i ? ", " : "", arr[i]);
    } else {
        for (long i = 0; i < HEAD; ++i) printf("%d, ", arr[i]);
        printf("...");
        for (long i = n - TAIL; i < n; ++i) printf(", %d", arr[i]);
    }
    printf("]\n");
}
