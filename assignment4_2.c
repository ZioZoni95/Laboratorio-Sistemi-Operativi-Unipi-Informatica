/*
Esercizio 2:
Scrivere un programma che prende in ingresso un intero 'N' (N < 512), alloca in memoria una matrice di NxN elementi di tipo float in modo contiguo.
Inizializzare la matrice come M1(i,j) = (i+j)/2.0. 
Salvare la matrice in due file separati:
 - 'mat_dump.dat' in formato binario
 - 'mat_dump.txt' in formato testuale

Scrivere un secondo programma o estendere quello precedente per leggere i file e memorizzare i dati in due matrici distinte (M1 ed M2).
Confrontare le due matrici con una funzione 'confronta' che usa un puntatore a funzione, utilizzando memcmp.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_N 512  // Limite massimo per N

void save_matrix_binary(const char *filename, float *matrix, int N) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Errore nell'apertura del file binario");
        exit(EXIT_FAILURE);
    }
    fwrite(matrix, sizeof(float), N * N, file);
    fclose(file);
}

void save_matrix_text(const char *filename, float *matrix, int N) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Errore nell'apertura del file testuale");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(file, "%f ", matrix[i * N + j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso corretto: %s <N>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int N = atoi(argv[1]);
    if (N <= 0 || N > MAX_N) {
        fprintf(stderr, "N deve essere compreso tra 1 e %d\n", MAX_N);
        return EXIT_FAILURE;
    }
    
    float *M1 = (float *)malloc(N * N * sizeof(float));
    if (!M1) {
        perror("Errore nell'allocazione della matrice");
        return EXIT_FAILURE;
    }
    
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            M1[i * N + j] = (i + j) / 2.0;
        }
    }
    
    save_matrix_binary("mat_dump.dat", M1, N);
    save_matrix_text("mat_dump.txt", M1, N);
    
    free(M1);
    printf("Matrice salvata con successo in 'mat_dump.dat' e 'mat_dump.txt'\n");
    return EXIT_SUCCESS;
}
