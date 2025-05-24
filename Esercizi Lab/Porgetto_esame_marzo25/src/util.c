#include <stdlib.h>
#include "util.h"

void dividi_array(int *array, int N, int k, int **gruppi) {
    int num_gruppi = N / (2 * k);
    
    for (int i = 0; i < num_gruppi; i++) {
        for (int j = 0; j < 2 * k; j++) {
            gruppi[i][j] = array[i * 2 * k + j];
        }
    }
}

int calcola_somma_parziale(int *gruppo, int k) {
    int somma = 0;
    for (int i = 0; i < 2 * k; i++) {
        somma += gruppo[i];
    }
    return somma;
}
