/**
 * @file myutils.c
 * @brief Implementazione di funzioni di utilità per l'ordinamento parallelo.
 *
 * Include la funzione di confronto per qsort, la funzione di merge
 * e una funzione per stampare l'array (utile per il debug).
 */

#include "myutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h> // Necessario per le asserzioni aggiunte per il debug

// NOTA: Includiamo common.h qui se DEBUG_PRINT_GEN fosse usato,
// ma per semplicità le stampe di debug dentro merge_sections usano printf.
// Se si volesse usare DEBUG_PRINT_GEN, includere "common.h".
// #include "common.h"

/**
 * @brief Funzione di confronto per qsort per ordinare interi.
 *
 * Confronta due interi puntati da a e b.
 * @param a Puntatore al primo intero.
 * @param b Puntatore al secondo intero.
 * @return <0 se *a < *b, 0 se *a == *b, >0 se *a > *b.
 */
int qsort_compare(const void *a, const void *b) {
    int int_a = *((int *)a);
    int int_b = *((int *)b);

    if (int_a < int_b) return -1;
    if (int_a > int_b) return 1;
    return 0;
    // Alternativa compatta: return (int_a > int_b) - (int_a < int_b);
}

/**
 * @brief Unisce due sezioni adiacenti e ordinate di un array sorgente in un array destinazione.
 *
 * Prende due sezioni ordinate [start1..end1] e [start2..end2] dall'array 'source'
 * e le unisce in modo ordinato nell'array 'dest', scrivendo nel range [start1..end2].
 * Si assume che end1 + 1 == start2 e che le sezioni sorgente siano già ordinate.
 * L'array 'dest' deve avere spazio sufficiente.
 *
 * @param source Array sorgente contenente le due sezioni ordinate.
 * @param dest Array destinazione dove scrivere il risultato unito.
 * @param start1 Indice iniziale della prima sezione in 'source'.
 * @param end1 Indice finale della prima sezione in 'source'.
 * @param start2 Indice iniziale della seconda sezione in 'source'.
 * @param end2 Indice finale della seconda sezione in 'source'.
 */
void merge_sections(int *source, int *dest, int start1, int end1, int start2, int end2) {
    int i = start1; // Indice per scorrere la prima sezione sorgente
    int j = start2; // Indice per scorrere la seconda sezione sorgente
    int k = start1; // Indice per scrivere nell'array di destinazione (inizia da start1)

    // Calcola l'indice finale atteso per la scrittura in dest.
    // Questo serve per le asserzioni, per verificare che non scriviamo fuori dai limiti previsti.
    // Nota: usiamo long per evitare potenziali overflow se gli intervalli fossero enormi,
    // anche se qui gli indici sono int.
    long num_elements1 = (end1 >= start1) ? (long)end1 - start1 + 1 : 0;
    long num_elements2 = (end2 >= start2) ? (long)end2 - start2 + 1 : 0;
    long expected_end_k = start1 + num_elements1 + num_elements2 - 1;

    // Stampa di debug opzionale per vedere gli input della funzione
    // printf("[DEBUG merge_sections] Start: Source[%d-%d] + Source[%d-%d] -> Dest[%d...%ld]\n",
    //        start1, end1, start2, end2, start1, expected_end_k);

    // Ciclo principale: confronta elementi dalle due sezioni finché una non è esaurita
    while (i <= end1 && j <= end2) {
        // --- Asserzione di Debug ---
        // Verifica che l'indice di scrittura 'k' sia all'interno del range atteso [start1 .. expected_end_k].
        // Se questa asserzione fallisce, significa che stiamo scrivendo fuori dai limiti previsti in 'dest'.
        assert(k >= start1 && k <= expected_end_k);

        if (source[i] <= source[j]) {
            dest[k] = source[i];
            // printf("  Merge write: dest[%d] = %d (from source[%d])\n", k, dest[k], i); // Debug scrittura
            i++;
        } else {
            dest[k] = source[j];
            // printf("  Merge write: dest[%d] = %d (from source[%d])\n", k, dest[k], j); // Debug scrittura
            j++;
        }
        k++; // Incrementa l'indice di destinazione dopo la scrittura
    }

    // Copia eventuali elementi rimanenti dalla prima sezione
    while (i <= end1) {
        // --- Asserzione di Debug ---
        assert(k >= start1 && k <= expected_end_k);
        dest[k] = source[i];
        // printf("  Merge write (rem i): dest[%d] = %d (from source[%d])\n", k, dest[k], i); // Debug scrittura
        i++; k++;
    }

    // Copia eventuali elementi rimanenti dalla seconda sezione
    while (j <= end2) {
        // --- Asserzione di Debug ---
        assert(k >= start1 && k <= expected_end_k);
        dest[k] = source[j];
        // printf("  Merge write (rem j): dest[%d] = %d (from source[%d])\n", k, dest[k], j); // Debug scrittura
        j++; k++;
    }

    // --- Asserzione Finale di Debug ---
    // Alla fine, l'indice 'k' dovrebbe puntare esattamente alla posizione *successiva* all'ultimo elemento scritto.
    // Quindi, k dovrebbe essere uguale a expected_end_k + 1.
    assert(k == expected_end_k + 1);

    // printf("[DEBUG merge_sections] End: Last write index k-1 = %d. Next index k = %d\n", k-1, k); // Debug opzionale
}


/**
 * @brief Stampa il contenuto dell'array (o una sua parte) a schermo.
 *
 * Utile per il debugging. Limita la stampa per array molto grandi
 * mostrando solo gli elementi iniziali e finali.
 *
 * @param label Etichetta da stampare prima dell'array.
 * @param arr Puntatore all'array da stampare.
 * @param n Numero di elementi nell'array.
 */
void print_array(const char *label, int *arr, long n) {
    // Limiti per evitare stampe eccessive su console
    const long MAX_PRINT_START = 15; // Max elementi da stampare all'inizio
    const long MAX_PRINT_END = 15;   // Max elementi da stampare alla fine
    const long TOTAL_MAX_PRINT = MAX_PRINT_START + MAX_PRINT_END + 1; // +1 per "..."

    printf("--- %s (N=%ld) ---\n", label, n);
    if (n <= 0) {
        printf("[]\n"); // Array vuoto
        printf("-------------------------\n");
        return;
    }

    printf("[");
    if (n <= TOTAL_MAX_PRINT) {
        // Stampa tutto l'array se è abbastanza piccolo
        for (long i = 0; i < n; ++i) {
            printf("%d%s", arr[i], (i < n - 1) ? ", " : "");
        }
    } else {
        // Stampa l'inizio...
        for (long i = 0; i < MAX_PRINT_START; ++i) {
            printf("%d, ", arr[i]);
        }
        printf("..."); // Separatore
        // Stampa la fine...
        for (long i = n - MAX_PRINT_END; i < n; ++i) {
             printf(", %d", arr[i]);
        }
    }
    printf("]\n-------------------------\n");
}