/**
 * @file myutils.c
 * @brief Implementazione di funzioni di utilità per l'ordinamento parallelo.
 */

#include "myutils.h" // Contiene la dichiarazione di merge_sections e qsort_compare
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>  // Necessario per le asserzioni

// common.h è idealmente incluso tramite myutils.h se myutils.h lo include,
// altrimenti assicurati che DEBUG e le macro di stampa siano disponibili.
// Se non lo è, aggiungi: #include "common.h" ma fai attenzione a inclusioni multiple.

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
 * @brief Unisce due sezioni adiacenti e ordinate di un array sorgente ('source')
 * in un array destinazione ('dest').
 * @param source Puntatore all'array da cui leggere i dati (già parzialmente ordinato).
 * @param dest Puntatore all'array temporaneo in cui scrivere il risultato del merge.
 * @param start1 Indice di inizio della prima sezione in 'source'.
 * @param end1 Indice di fine della prima sezione in 'source'.
 * @param start2 Indice di inizio della seconda sezione in 'source'.
 * @param end2 Indice di fine della seconda sezione in 'source'.
 * @param N_total Dimensione totale dell'array originale (usato per validare gli indici con assert).
 *
 * IMPORTANTE: La sezione di output in 'dest' andrà da 'dest[start1]' fino
 * all'indice corrispondente alla fine combinata dei due blocchi.
 */
void merge_sections(int *source, int *dest,
                    int start1, int end1,
                    int start2, int end2,
                    long N_total) {

    // -------------- INIZIO BLOCCO STAMPE DI DEBUG PER MERGE_SECTIONS --------------
    // Attiva queste stampe per un debug granulare. Filtra per casi specifici se l'output è troppo.
    // Esempio di condizione di filtro (P=8, k=0, chiamata da W1 che gestisce source[2..3]):
    // if (start1 == 2 && end1 == 2 && start2 == 3 && end2 == 3 && N_total == 8) {
    // Oppure stampa sempre se DEBUG è alto, ma preparati a molto output.
    #if DEBUG // Usa le tue macro DEBUG_PRINT_GEN o printf semplici
    printf("[MERGE_SECTIONS DEBUG ENTRY] ThreadID(esterno): ? (Passare tid se possibile per contesto)\n"); // Sarebbe utile avere il tid qui
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
    fflush(stdout);
    #endif
    // -------------- FINE BLOCCO STAMPE DI DEBUG PER MERGE_SECTIONS --------------

    // Asserzioni robuste come prima...
    if ( (end1 >= start1) || (end2 >= start2) ) {
        assert(N_total > 0);
    }
    // ... (mantieni tutte le tue asserzioni esistenti) ...
    if (end1 >= start1) { 
        assert(start1 >= 0 && start1 <= end1 && end1 < N_total);
    }
    if (end2 >= start2) { 
        assert(start2 >= 0 && start2 <= end2 && end2 < N_total);
        if (end1 >= start1) {
            assert(start2 == end1 + 1);
        }
    }


    int i = start1; 
    int j = start2; 
    int k = start1; 

    long num_elements1 = (end1 >= start1) ? (long)end1 - start1 + 1 : 0;
    long num_elements2 = (end2 >= start2) ? (long)end2 - start2 + 1 : 0;

    if (num_elements1 == 0 && num_elements2 == 0) {
        #if DEBUG
        printf("[MERGE_SECTIONS s1=%d] Entrambi i blocchi sorgente vuoti. Uscita.\n", start1); fflush(stdout);
        #endif
        return;
    }
    
    long expected_end_k = start1 + num_elements1 + num_elements2 - 1;
    assert(start1 >= 0);
    if (num_elements1 + num_elements2 > 0) {
        assert(expected_end_k < N_total);
    }

    #if DEBUG
    printf("[MERGE_SECTIONS PRE-LOOP s1=%d] i=%d, j=%d, k_start_write=%d, expected_end_write=%ld\n",
           start1, i, j, k, expected_end_k);
    fflush(stdout);
    #endif

    while ( (num_elements1 > 0 && i <= end1) && (num_elements2 > 0 && j <= end2) ) {
        assert(k >= start1 && k <= expected_end_k);
        assert(i >= start1 && i <= end1); 
        assert(j >= start2 && j <= end2); 

        int val_i = source[i];
        int val_j = source[j];
        int val_to_write;

        if (val_i <= val_j) {
            val_to_write = val_i;
            i++;
        } else {
            val_to_write = val_j;
            j++;
        }
        #if DEBUG
        // Condiziona questa stampa se diventa troppo verbosa, es. per un tid specifico
        printf("[MERGE_SECTIONS LOOP s1=%d] k_write=%d: Scrivo %d (da src_i=%d o src_j=%d). Prox i=%d,j=%d\n",
               start1, k, val_to_write, source[ (val_to_write == val_i) ? i-1 : i ], source[ (val_to_write == val_j) ? j-1 : j], i, j);
        fflush(stdout);
        #endif
        dest[k++] = val_to_write;
    }

    while (num_elements1 > 0 && i <= end1) {
        assert(k >= start1 && k <= expected_end_k);
        assert(i >= start1 && i <= end1);
        #if DEBUG
        printf("[MERGE_SECTIONS REM1 s1=%d] k_write=%d: Scrivo %d (da src[%d])\n", start1, k, source[i], i); fflush(stdout);
        #endif
        dest[k++] = source[i++];
    }

    while (num_elements2 > 0 && j <= end2) {
        assert(k >= start1 && k <= expected_end_k);
        assert(j >= start2 && j <= end2);
        #if DEBUG
        printf("[MERGE_SECTIONS REM2 s1=%d] k_write=%d: Scrivo %d (da src[%d])\n", start1, k, source[j], j); fflush(stdout);
        #endif
        dest[k++] = source[j++];
    }

    if (num_elements1 + num_elements2 > 0) {
        assert(k == (expected_end_k + 1));
    } else {
        assert(k == start1);
    }

    #if DEBUG
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
 * Migliorata per gestire array vuoti o N non positivo e puntatori NULL.
 */
void print_array(const char *label, int *arr, long n) {
    const long MAX_PRINT_START = 15; // Stampa i primi N elementi
    const long MAX_PRINT_END = 15;   // Stampa gli ultimi N elementi
    // Stampa completo se n <= MAX_PRINT_START + MAX_PRINT_END + (spazio per "...")
    const long TOTAL_MAX_PRINT = MAX_PRINT_START + MAX_PRINT_END + 5; 

    printf("--- %s (N=%ld) ---\n", label, n);
    if (n <= 0) {
        printf("[] (Array vuoto o N non positivo)\n");
    } else if (arr == NULL) {
        // Questo caso non dovrebbe accadere se la logica a monte è corretta, ma è un buon controllo.
        printf("[Errore: Tentativo di stampare un array NULL con N=%ld]\n", n);
    } else {
        printf("[");
        if (n <= TOTAL_MAX_PRINT) { // Se l'array è abbastanza piccolo, stampalo tutto
            for (long i = 0; i < n; ++i) {
                printf("%d%s", arr[i], (i < n - 1) ? ", " : "");
            }
        } else { // Altrimenti, stampa l'inizio, "..." e la fine
            for (long i = 0; i < MAX_PRINT_START; ++i) {
                printf("%d, ", arr[i]);
            }
            printf("...");
            for (long i = n - MAX_PRINT_END; i < n; ++i) {
                 printf(", %d", arr[i]);
            }
        }
        printf("]");
    }
    printf("\n-------------------------\n");
    fflush(stdout); // Assicura che tutte le stampe siano visibili immediatamente
}