/**
 * @file myutils.c
 * @brief Implementazione di funzioni di utilità per l'ordinamento parallelo.
 *
 * Questo file contiene:
 * - qsort_compare: una funzione di confronto standard per qsort per interi.
 * - merge_sections: la logica chiave per unire due sezioni ordinate di un array sorgente
 * in un array destinazione. Include stampe di debug dettagliate se DEBUG è attivo.
 * - print_array: una funzione per stampare il contenuto di un array, con gestione
 * per array grandi (stampa solo inizio e fine) e casi limite (array nullo o vuoto).
 */

#include "myutils.h" // Contiene la dichiarazione di merge_sections e qsort_compare
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>  

/**
 * @brief Funzione di confronto per qsort per ordinare interi in ordine ascendente.
 * @param a Puntatore al primo elemento da confrontare.
 * @param b Puntatore al secondo elemento da confrontare.
 * @return -1 se int_a < int_b
 * @return  1 se int_a > int_b
 * @return  0 se int_a == int_b
 */
int qsort_compare(const void *a, const void *b) {
    int int_a = *((int *)a); // Dereferenzia e casta a intero
    int int_b = *((int *)b); // Dereferenzia e casta a intero

    if (int_a < int_b) return -1;
    if (int_a > int_b) return 1;
    return 0;
}

/**
 * @brief Unisce due sezioni adiacenti e ordinate di un array sorgente ('source')
 * in un array destinazione ('dest').
 *
 * @param source Puntatore all'array da cui leggere i dati (già parzialmente ordinato,
 * le sezioni [start1, end1] e [start2, end2] devono essere ordinate).
 * @param dest Puntatore all'array temporaneo in cui scrivere il risultato del merge.
 * La sezione di output in 'dest' andrà da 'dest[start1]' fino
 * all'indice corrispondente alla fine combinata dei due blocchi.
 * @param start1 Indice di inizio della prima sezione in 'source'.
 * @param end1 Indice di fine della prima sezione in 'source'.
 * @param start2 Indice di inizio della seconda sezione in 'source'.
 * @param end2 Indice di fine della seconda sezione in 'source'.
 * @param N_total Dimensione totale dell'array originale (usato per validare gli indici con assert).
 *
 * @note Le asserzioni sono usate per verificare la validità degli indici e delle condizioni
 * pre-esecuzione, specialmente se DEBUG è attivo.
 * @note Le stampe di DEBUG (attivate da #if DEBUG) sono molto dettagliate per tracciare
 * l'esecuzione del merge passo-passo per verifiche.
 */
void merge_sections(int *source, int *dest,
                    int start1, int end1,
                    int start2, int end2,
                    long N_total) {

    // -------------- INIZIO BLOCCO STAMPE DI DEBUG PER MERGE_SECTIONS --------------
    #if DEBUG
    // Queste stampe sono attive solo se DEBUG è settato 1
    // Forniscono informazioni dettagliate sugli input e lo stato del merge.
    printf("[MERGE_SECTIONS DEBUG ENTRY] (TID non disponibile direttamente qui)\n");
    printf("  Parametri: source=%p, dest=%p, s1=%d, e1=%d, s2=%d, e2=%d, N_total=%ld\n",
           (void*)source, (void*)dest, start1, end1, start2, end2, N_total);
    
    long dbg_num_elements1_calc = (end1 >= start1) ? (long)end1 - start1 + 1 : 0;
    long dbg_num_elements2_calc = (end2 >= start2) ? (long)end2 - start2 + 1 : 0;

    printf("  Source Block1 (indices %d-%d, %ld elementi): ", start1, end1, dbg_num_elements1_calc);
    if(dbg_num_elements1_calc > 0 && start1 >=0 && end1 < N_total) for(int x=start1; x<=end1; ++x) printf("%d ", source[x]); else printf("(vuoto o non valido)");
    printf("\n");

    printf("  Source Block2 (indices %d-%d, %ld elementi): ", start2, end2, dbg_num_elements2_calc);
    if(dbg_num_elements2_calc > 0 && start2 >=0 && end2 < N_total) for(int x=start2; x<=end2; ++x) printf("%d ", source[x]); else printf("(vuoto o non valido)");
    printf("\n");
    fflush(stdout); // Assicura che le stampe di debug appaiano immediatamente
    #endif
    // -------------- FINE BLOCCO STAMPE DI DEBUG PER MERGE_SECTIONS --------------

    // Asserzioni per la validità degli input. Cruciali per garantire precondizioni.
    // Se una qualsiasi di queste condizioni non è vera, il programma terminerà (se le asserzioni sono attive).
    if ( (end1 >= start1) || (end2 >= start2) ) { // Se almeno una delle due sezioni ha elementi
        assert(N_total > 0); // N_total deve essere positivo
    }
    if (end1 >= start1) { // Se la prima sezione ha elementi
        assert(start1 >= 0 && start1 <= end1 && end1 < N_total); // Indici validi per la prima sezione
    }
    if (end2 >= start2) { // Se la seconda sezione ha elementi
        assert(start2 >= 0 && start2 <= end2 && end2 < N_total); // Indici validi per la seconda sezione
        if (end1 >= start1) { // Se anche la prima sezione ha elementi
            assert(start2 == end1 + 1); // Le due sezioni devono essere adiacenti
        }
    }

    // Indici per scorrere le due sezioni sorgente (i, j) e la sezione destinazione (k)
    int i = start1; // Indice per la prima sezione (source[start1...end1])
    int j = start2; // Indice per la seconda sezione (source[start2...end2])
    int k = start1; // Indice per l'array destinazione (dest[start1...])

    // Calcola il numero di elementi in ciascuna sezione sorgente
    long num_elements1 = (end1 >= start1) ? (long)end1 - start1 + 1 : 0;
    long num_elements2 = (end2 >= start2) ? (long)end2 - start2 + 1 : 0;

    // Se entrambe le sezioni sorgente sono vuote, non c'è nulla da unire.
    if (num_elements1 == 0 && num_elements2 == 0) {
        #if DEBUG
        printf("[MERGE_SECTIONS s1=%d] Entrambi i blocchi sorgente vuoti. Uscita.\n", start1); fflush(stdout);
        #endif
        return; // Termina la funzione
    }
    
    // Calcola l'indice finale atteso nell'array di destinazione dopo il merge.
    long expected_end_k = start1 + num_elements1 + num_elements2 - 1;
    assert(start1 >= 0); // L'indice di inizio della scrittura deve essere non negativo.
    if (num_elements1 + num_elements2 > 0) { // Se c'è almeno un elemento da scrivere
        assert(expected_end_k < N_total); // L'indice finale di scrittura non deve superare N_total.
    }

    #if DEBUG
    printf("[MERGE_SECTIONS PRE-LOOP s1=%d] i=%d, j=%d, k_start_write=%d, expected_end_write=%ld\n",
           start1, i, j, k, expected_end_k);
    fflush(stdout);
    #endif

    // Loop principale del merge: confronta elementi dalle due sezioni sorgente
    // e copia il minore nell'array destinazione. Continua finché ci sono elementi
    // in entrambe le sezioni.
    while ( (num_elements1 > 0 && i <= end1) && (num_elements2 > 0 && j <= end2) ) {
        // Asserzioni per la validità degli indici durante il loop (difesa contro off-by-one errors)
        assert(k >= start1 && k <= expected_end_k); // k deve essere nell'intervallo di scrittura atteso
        assert(i >= start1 && i <= end1);           // i deve essere nell'intervallo della prima sezione
        assert(j >= start2 && j <= end2);           // j deve essere nell'intervallo della seconda sezione

        int val_i = source[i]; // Valore corrente dalla prima sezione
        int val_j = source[j]; // Valore corrente dalla seconda sezione
        int val_to_write;      // Valore da scrivere in dest[k]

        if (val_i <= val_j) { // Se l'elemento dalla prima sezione è minore o uguale
            val_to_write = val_i;
            i++; // Avanza l'indice per la prima sezione
        } else { // Se l'elemento dalla seconda sezione è minore
            val_to_write = val_j;
            j++; // Avanza l'indice per la seconda sezione
        }
        #if DEBUG
        // Stampa di debug per tracciare quale valore viene scritto e da dove proviene
        printf("[MERGE_SECTIONS LOOP s1=%d] k_write=%d: Scrivo %d (da src_i=%d o src_j=%d). Prox i=%d,j=%d\n",
               start1, k, val_to_write, 
               (val_to_write == val_i && i > start1) ? source[i-1] : ( (val_to_write == val_i) ? val_i : -1 ), // Logica complessa per stampare il valore letto
               (val_to_write == val_j && j > start2) ? source[j-1] : ( (val_to_write == val_j) ? val_j : -1 ),
               i, j);
        fflush(stdout);
        #endif
        dest[k++] = val_to_write; // Scrive il valore scelto in dest e avanza k
    }

    // Dopo il loop principale, una delle due sezioni sorgente potrebbe avere elementi rimanenti.
    // Copia gli eventuali elementi rimanenti dalla prima sezione.
    while (num_elements1 > 0 && i <= end1) {
        assert(k >= start1 && k <= expected_end_k);
        assert(i >= start1 && i <= end1);
        #if DEBUG
        printf("[MERGE_SECTIONS REM1 s1=%d] k_write=%d: Scrivo %d (da src[%d])\n", start1, k, source[i], i); fflush(stdout);
        #endif
        dest[k++] = source[i++];
    }

    // Copia gli eventuali elementi rimanenti dalla seconda sezione.
    while (num_elements2 > 0 && j <= end2) {
        assert(k >= start1 && k <= expected_end_k);
        assert(j >= start2 && j <= end2);
        #if DEBUG
        printf("[MERGE_SECTIONS REM2 s1=%d] k_write=%d: Scrivo %d (da src[%d])\n", start1, k, source[j], j); fflush(stdout);
        #endif
        dest[k++] = source[j++];
    }

    // Verifica finale: l'indice k dovrebbe ora puntare all'elemento successivo
    // all'ultimo elemento scritto.
    if (num_elements1 + num_elements2 > 0) { // Se sono stati scritti elementi
        assert(k == (expected_end_k + 1));
    } else { // Se non sono stati scritti elementi (entrambe le sezioni erano vuote)
        assert(k == start1); // k non si è mosso da start1
    }

    #if DEBUG
    // Stampa il contenuto della sezione di destinazione dopo il merge
    printf("[MERGE_SECTIONS EXIT s1=%d] Contenuto finale di dest[%d...%ld]: ", start1, start1, expected_end_k);
    if(num_elements1 + num_elements2 > 0 && start1 >=0 && expected_end_k < N_total) {
        for(long x_dbg=start1; x_dbg <= expected_end_k; ++x_dbg) printf("%d ", dest[x_dbg]);
    } else {
        printf("(intervallo non valido o vuoto per la stampa)");
    }
    printf("\n");
    fflush(stdout);
    #endif
}

/**
 * @brief Stampa il contenuto dell'array (o una sua parte) a schermo.
 *
 * Se l'array è più grande di una certa soglia (TOTAL_MAX_PRINT), stampa solo
 * i primi MAX_PRINT_START elementi, un "..." e gli ultimi MAX_PRINT_END elementi.
 * Gestisce anche i casi di array nullo o con N non positivo.
 *
 * @param label Etichetta testuale da stampare prima dell'array.
 * @param arr Puntatore all'array di interi da stampare.
 * @param n Numero di elementi nell'array.
 */
void print_array(const char *label, int *arr, long n) {
    const long MAX_PRINT_START = 15; // Numero di elementi da stampare all'inizio per array grandi
    const long MAX_PRINT_END = 15;   // Numero di elementi da stampare alla fine per array grandi
    // Soglia per decidere se stampare l'array completo o in forma abbreviata
    const long TOTAL_MAX_PRINT = MAX_PRINT_START + MAX_PRINT_END + 5; // Il +5 è per "..., " e la formattazione

    printf("--- %s (N=%ld) ---\n", label, n); // Stampa l'etichetta e la dimensione N
    if (n <= 0) { // Se l'array è vuoto o N non è positivo
        printf("[] (Array vuoto o N non positivo)\n");
    } else if (arr == NULL) { // Se il puntatore all'array è nullo (caso di errore)
        printf("[Errore: Tentativo di stampare un array NULL con N=%ld]\n", n);
    } else { // Array valido e con elementi
        printf("[");
        if (n <= TOTAL_MAX_PRINT) { // Se l'array è abbastanza piccolo, stampalo tutto
            for (long i = 0; i < n; ++i) {
                printf("%d%s", arr[i], (i < n - 1) ? ", " : ""); // Stampa elemento e virgola (tranne per l'ultimo)
            }
        } else { // Altrimenti, stampa l'inizio, "..." e la fine
            // Stampa i primi MAX_PRINT_START elementi
            for (long i = 0; i < MAX_PRINT_START; ++i) {
                printf("%d, ", arr[i]);
            }
            printf("..."); // Separatore
            // Stampa gli ultimi MAX_PRINT_END elementi
            for (long i = n - MAX_PRINT_END; i < n; ++i) {
                 printf(", %d", arr[i]); // Virgola prima di ogni elemento in questa parte
            }
        }
        printf("]"); // Chiusura parentesi quadra
    }
    printf("\n-------------------------\n");
    fflush(stdout); // Assicura che l'output sia visibile immediatamente (utile per pipe o redirect)
}