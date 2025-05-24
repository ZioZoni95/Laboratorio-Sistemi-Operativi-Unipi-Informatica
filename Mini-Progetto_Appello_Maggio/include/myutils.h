#ifndef MYUTILS_H
#define MYUTILS_H

#include "common.h" // Per accesso a tipi base se necessario

// --- Dichiarazioni Funzioni Utilità ---

// Funzione di Comparazione richiesta da qsort per ordinare interi.
int qsort_compare(const void *a, const void *b);

// Funzione Helper per la fase di Merge.
// Unisce due sezioni adiacenti e ordinate (start1..end1, start2..end2)
// presenti nell'array 'source' e scrive il risultato ordinato
// nell'array 'dest' nella sezione combinata start1..end2.
void merge_sections(int *source, int *dest, int start1, int end1, int start2, int end2, long N_total);

// Funzione per stampare l'array (o una sua parte se N è grande).
// Utile per il debug e per visualizzare lo stato dell'ordinamento.
void print_array(const char *label, int *arr, long n);

#endif // MYUTILS_H