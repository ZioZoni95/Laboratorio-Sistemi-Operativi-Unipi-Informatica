#include "myutils.h"
#include <stdio.h>
#include <stdlib.h>

// Implementazione qsort_compare
int qsort_compare(const void *a, const void *b) {
    int int_a = *((int *)a);
    int int_b = *((int *)b);
    // Restituisce <0 se a < b, 0 se a == b, >0 se a > b
    if (int_a < int_b) return -1;
    if (int_a > int_b) return 1;
    return 0;
}

// Implementazione merge_sections
void merge_sections(int *source, int *dest, int start1, int end1, int start2, int end2) {
    int i = start1; // Indice per la prima sezione sorgente
    int j = start2; // Indice per la seconda sezione sorgente
    int k = start1; // Indice per l'array di destinazione

    // Confronta elementi dalle due sezioni finché una non è esaurita
    while (i <= end1 && j <= end2) {
        if (source[i] <= source[j]) {
            dest[k++] = source[i++]; // Copia elemento minore da sezione 1
        } else {
            dest[k++] = source[j++]; // Copia elemento minore da sezione 2
        }
    }

    // Copia eventuali elementi rimanenti dalla prima sezione
    while (i <= end1) {
        dest[k++] = source[i++];
    }

    // Copia eventuali elementi rimanenti dalla seconda sezione
    while (j <= end2) {
        dest[k++] = source[j++];
    }
}

// Implementazione print_array
void print_array(const char *label, int *arr, long n) {
    // Limite per evitare stampe enormi
    const long MAX_PRINT_START = 15; // Max elementi da stampare all'inizio
    const long MAX_PRINT_END = 15;   // Max elementi da stampare alla fine
    const long TOTAL_MAX_PRINT = MAX_PRINT_START + MAX_PRINT_END;

    printf("--- %s (N=%ld) ---\n", label, n);
    if (n <= 0) {
        printf("[]\n");
        return;
    }

    printf("[");
    if (n <= TOTAL_MAX_PRINT) {
        // Stampa tutto l'array se è abbastanza piccolo
        for (long i = 0; i < n; ++i) {
            printf("%d%s", arr[i], (i < n - 1) ? ", " : "");
        }
    } else {
        // Stampa inizio...
        for (long i = 0; i < MAX_PRINT_START; ++i) {
            printf("%d, ", arr[i]);
        }
        printf("...");
        // Stampa fine...
        for (long i = n - MAX_PRINT_END; i < n; ++i) {
             printf(", %d", arr[i]);
        }
    }
    printf("]\n-------------------------\n");
}