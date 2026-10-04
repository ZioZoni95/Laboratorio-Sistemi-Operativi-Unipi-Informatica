#ifndef MYUTILS_H
#define MYUTILS_H

#include "common.h"

// Funzione di confronto per qsort: ordina interi in senso crescente.
int qsort_compare(const void *a, const void *b);

// Primo indice della partizione i-esima quando n elementi sono divisi in `parts`
// partizioni contigue (le prime n % parts hanno un elemento in più).
// Vale per 0 <= i <= parts: la partizione i è l'intervallo [bound(i), bound(i+1)),
// e bound(parts) == n. Se n < parts alcune partizioni sono vuote.
long partition_bound(long i, long n, long parts);

// Numero di passi di merge necessari per fondere `parts` partizioni: ceil(log2(parts)).
// Per parts = 1 non servono merge (0 passi).
int merge_steps(long parts);

// Fonde le due sezioni ordinate adiacenti src[lo..mid) e src[mid..hi) in dst[lo..hi).
// Intervalli semiaperti; sezioni vuote ammesse (se mid == hi si copia soltanto src[lo..mid)).
// `src` e `dst` devono essere buffer distinti.
void merge_sections(const int *src, int *dst, long lo, long mid, long hi);

// Stampa l'array (o inizio e fine se è lungo) con un'etichetta.
void print_array(const char *label, const int *arr, long n);

#endif // MYUTILS_H
