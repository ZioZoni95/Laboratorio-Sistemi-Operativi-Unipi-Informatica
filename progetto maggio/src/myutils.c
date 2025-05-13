/**
 * @file myutils.c
 * @brief Implementazione di funzioni di utilità per l'ordinamento parallelo.
 */

#include "myutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h> // Necessario per le asserzioni

// (common.h è già incluso tramite myutils.h)

/**
 * @brief Funzione di confronto per qsort per ordinare interi.
 */
int qsort_compare(const void *a, const void *b) {
    int int_a = *((int *)a);
    int int_b = *((int *)b);

    if (int_a < int_b) return -1;
    if (int_a > int_b) return 1;
    return 0;
}

/**
 * @brief Unisce due sezioni adiacenti e ordinate di un array sorgente in un array destinazione.
 */
void merge_sections(int *source, int *dest,
                    int start1, int end1,
                    int start2, int end2,
                    long N_total) { // !!! MODIFICA: Aggiunto N_total

    // --- Asserzioni Iniziali Robuste ---
    // Controlla che N_total sia positivo se gli intervalli hanno elementi.
    // Se gli intervalli sono vuoti, N_total potrebbe essere 0 (array vuoto gestito).
    if ( (end1 >= start1) || (end2 >= start2) ) { // Se c'è almeno un elemento da processare
        assert(N_total > 0 && "N_total deve essere positivo se ci sono elementi da unire.");
    }

    // Controlli sul primo blocco (source[start1..end1])
    // Se il blocco ha elementi, gli indici devono essere validi e ordinati.
    if (end1 >= start1) { // Il blocco 1 ha elementi
        assert(start1 >= 0 && "start1 deve essere >= 0.");
        assert(end1 < N_total && "end1 deve essere < N_total.");
        assert(start1 <= end1 && "start1 deve essere <= end1 per un blocco valido.");
    }

    // Controlli sul secondo blocco (source[start2..end2])
    // Se il blocco ha elementi, gli indici devono essere validi e ordinati.
    // E il secondo blocco deve iniziare dopo la fine del primo.
    if (end2 >= start2) { // Il blocco 2 ha elementi
        assert(start2 >= 0 && "start2 deve essere >= 0.");
        assert(end2 < N_total && "end2 deve essere < N_total.");
        assert(start2 <= end2 && "start2 deve essere <= end2 per un blocco valido.");
        // Assicura che i blocchi siano adiacenti o che il secondo inizi dopo il primo.
        // Se il primo blocco ha elementi, il secondo deve iniziare dopo.
        if (end1 >= start1) {
            assert(start2 == end1 + 1 && "I blocchi devono essere strettamente adiacenti se entrambi non vuoti.");
        }
    }

    int i = start1; // Indice per scorrere la prima sezione sorgente
    int j = start2; // Indice per scorrere la seconda sezione sorgente
    int k = start1; // Indice per scrivere nell'array di destinazione (inizia da start1)

    // Calcola l'indice finale atteso per la scrittura in dest.
    long num_elements1 = (end1 >= start1) ? (long)end1 - start1 + 1 : 0;
    long num_elements2 = (end2 >= start2) ? (long)end2 - start2 + 1 : 0;

    // Se non ci sono elementi in nessuno dei due blocchi, non c'è nulla da fare.
    if (num_elements1 == 0 && num_elements2 == 0) {
        return;
    }

    long expected_end_k = start1 + num_elements1 + num_elements2 - 1;
    // Verifica che l'intervallo di scrittura previsto per 'dest' sia valido.
    assert(start1 >= 0 && "L'indice di inizio scrittura k (start1) deve essere >= 0.");
    // Se ci sono elementi da scrivere, expected_end_k deve essere entro i limiti.
    if (num_elements1 + num_elements2 > 0) {
        assert(expected_end_k < N_total && "L'intervallo di scrittura in dest supera N_total.");
    }


    // Stampa di debug opzionale per vedere gli input della funzione
    // DEBUG_PRINT_GEN("[DEBUG merge_sections] Source[%d-%d](%ld el) + Source[%d-%d](%ld el) -> Dest[%d...%ld]",
    //        start1, end1, num_elements1, start2, end2, num_elements2, start1, expected_end_k);

    // Ciclo principale: confronta elementi dalle due sezioni finché una non è esaurita
    // o finché entrambi i blocchi hanno elementi validi da considerare.
    while ( (num_elements1 > 0 && i <= end1) && (num_elements2 > 0 && j <= end2) ) {
        assert(k >= start1 && k <= expected_end_k && "Indice k fuori range durante il merge principale.");
        assert(i >= start1 && i <= end1); // Legge da Blocco1 valido
        assert(j >= start2 && j <= end2); // Legge da Blocco2 valido

        if (source[i] <= source[j]) {
            dest[k++] = source[i++];
        } else {
            dest[k++] = source[j++];
        }
    }

    // Copia eventuali elementi rimanenti dalla prima sezione
    while (num_elements1 > 0 && i <= end1) {
        assert(k >= start1 && k <= expected_end_k && "Indice k fuori range copiando rimanenti da Blocco1.");
        assert(i >= start1 && i <= end1); // Legge da Blocco1 valido
        dest[k++] = source[i++];
    }

    // Copia eventuali elementi rimanenti dalla seconda sezione
    while (num_elements2 > 0 && j <= end2) {
        assert(k >= start1 && k <= expected_end_k && "Indice k fuori range copiando rimanenti da Blocco2.");
        assert(j >= start2 && j <= end2); // Legge da Blocco2 valido
        dest[k++] = source[j++];
    }

    // Asserzione Finale: k dovrebbe puntare all'elemento successivo all'ultimo scritto.
    // Cioè, k dovrebbe essere expected_end_k + 1.
    // Questa asserzione è valida solo se c'era almeno un elemento da unire.
    if (num_elements1 + num_elements2 > 0) {
        assert(k == (expected_end_k + 1) && "k non è nella posizione finale attesa.");
    } else { // Se non c'erano elementi, k non dovrebbe essersi mosso da start1.
        assert(k == start1 && "k si è mosso ma non c'erano elementi da unire.");
    }
}


/**
 * @brief Stampa il contenuto dell'array (o una sua parte) a schermo.
 */
void print_array(const char *label, int *arr, long n) {
    const long MAX_PRINT_START = 15;
    const long MAX_PRINT_END = 15;
    const long TOTAL_MAX_PRINT = MAX_PRINT_START + MAX_PRINT_END + 1;

    printf("--- %s (N=%ld) ---\n", label, n);
    if (n <= 0) {
        printf("[] (Array vuoto o N non positivo)\n");
        printf("-------------------------\n");
        fflush(stdout); // Assicura che la stampa sia visibile
        return;
    }
    if (arr == NULL && n > 0) {
        printf("[Errore: Puntatore array NULL con N=%ld]\n", n);
        printf("-------------------------\n");
        fflush(stdout);
        return;
    }


    printf("[");
    if (n <= TOTAL_MAX_PRINT) {
        for (long i = 0; i < n; ++i) {
            printf("%d%s", arr[i], (i < n - 1) ? ", " : "");
        }
    } else {
        for (long i = 0; i < MAX_PRINT_START; ++i) {
            printf("%d, ", arr[i]);
        }
        printf("...");
        for (long i = n - MAX_PRINT_END; i < n; ++i) {
             printf(", %d", arr[i]);
        }
    }
    printf("]\n-------------------------\n");
    fflush(stdout); // Assicura che la stampa sia visibile
}