/*
Esercizio 4: numeri random
Generare N numeri casuali interi nell'intervallo [K1, K2[ utilizzando la funzione rand_r().
N, K1 e K2 sono passati come argomenti del main opzionali, e se non forniti dall'utente assumono valori di default sulla base di opportune #define (es. N=1000 K1=100 K2=120).
Il programma deve restituire il numero di occorrenze di ciascun intero i nell'intervallo [K1, K2[ e stamparle sullo standard output.

Esempio di output con K1=0 e K2=10:

Occorrenze di:
0  : 10.25% 
1  :  9.97% 
2  :  9.48% 
3  :  9.77% 
4  : 10.19% 
5  : 10.93% 
6  :  9.80% 
7  :  9.93% 
8  : 10.00% 
9  :  9.68%
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//Definizioni di default per i parametri
#define N 1000 //Numero di numeri casuali da generare
#define K1 100 //Lower bound dell'intervallo (inclusivo)
#define K2 120 //Upper bound dell'intervallo (esclusivo)

int main(int argc, char *argv[]){
    int n = N; //Numero di numeri casuali da generare
    int k1 = K1; //Lower bound dell'intervallo (inclusivo)
    int k2 = K2; //Upper bound dell'intervallo (esclusivo)

    //Gestione degli argomenti forniti
    if(argc < 1){
        n = atoi(argv[1]); //Se fornito, usa il primo argomento per N
    }

    if(argc > 2){
        k1 = atoi(argv[2]); //Se fornito, usa il secondo argomento per K1
    }

    if (argc > 3){
        k2 = atoi(argv[3]); //Se fornito, usa il terzo argomento per K2
    }

    //Verifica che K1 sia minore di K2 se l'utente ha fornito argomenti
    if(k1 >= k2){
        fprintf(stderr, "Errore: K1 deve essere minore di K2.\n");
        return 1;
    }

    //Calcolo la dimensione dell'intervallo [K1, K2[
    int range = k2 - k1;

    //Array per memorizzare il numero di occorrenze per ciascun numero nell'intervallo
    int count[range];

    //Inizializzo l'array a zero
    for(int i = 0; i < range; i++){
        count[i] = 0;
    }
    //Impostazione del seme per rand_r() utilizzando l'ora corrente
    unsigned int seed = (unsigned int)time(NULL);

    //Generazione di N numeri casuali e conteggio delle occorrenze
    for(int i = 0; i < n; i++){
        //Genera un numero random nell'intervallo k1, k2 escluso
        int num = k1 + (rand_r(&seed) % range);

        //incrementa il conratore per il numero generato
        count[num - k1]++;
    }

    //Stampa delle percentuali di occorrenza di ciascun numero nell'intervallo
    printf("Occorrenze di:\n");
    for(int i = 0; i < range; i++){
        //Calcola la percentuale di occorenza del numero k1 + i
        double percentage = ((double)count[i] / n) * 100;

        //Stampa il numero e la sua percentuale di occorrenza
        printf("%d : %.2f%%\n", k1 + i, percentage);
    }
    return 0;
}