/*
 * Esercizio 2: Puntatori a Funzioni
 * 
 * Questo programma estende l'esercizio precedente utilizzando puntatori a funzione per gestire le opzioni della riga di comando.
 * 
 * - Deve riconoscere le opzioni '-n', '-m', '-o', '-h'.
 * - Ogni opzione deve essere gestita da una specifica funzione dedicata.
 * - Le funzioni vengono memorizzate in un array di puntatori a funzione 'V'.
 * - Quando un'opzione viene riconosciuta, viene chiamata la funzione corrispondente dal vettore 'V'.
 * - Se l'opzione non è valida, il programma stampa un errore.
 * - L'opzione '-h' mostra un messaggio di utilizzo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

// Funzione per stampare il messaggio di uso del programma
void print_usage() {
    printf("Uso: nome-programma -n <num. intero> -m <num. intero> -o <stringa> -h\n");
}

//Funzione per gestire l'opzione '-n'
int arg_n(const char *arg){
    int value = atoi(arg); //Converte l'argomento in intero
    printf("Funzione arg_n %d\n", value);
    return (value == 0 && arg[0] != '0') ? -1 : 0; //Controlla l'errore nella conversione
}

//Funzione per gestire l'opzione '-m'
int arg_m(const char *arg){
    int value = atoi(arg);
    printf("Funzione arg_m %d\n", value);
    return (value == 0 && arg[0] != '0') ? -1 : 0;
}

//Funzione per gestire l'opzione '-o'
int arg_o(const char *arg){
    printf("Funzione arg_o: %s\n",arg);
    return 0;
}

//Funzione per gestire l'opzione '-h' di help
int arg_h(const char *arg){
    (void)arg; //Evita warning per l'argomento non utilizzato
    print_usage();
    return 0;
}

//DEBUG
// Funzione per mappare l'opzione getopt al corretto indice dell'array V
int opt_index(char opt) {
    switch (opt) {
        case 'n': return 0;
        case 'm': return 1;
        case 'o': return 2;
        case 'h': return 3;
        default: return -1; // Errore
    }
}

int main(int argc, char *argv[]){
    //Controllo che siano passati argomenti validi
    if(argc < 2){
        print_usage();
        return -1;
    }

    //Dichiarazione e inizializzazione del vettore di puntatori a funzione
    int(*V[4])(const char*) = {arg_n, arg_m, arg_o, arg_h};
    
    int opt; //Variabile pe immagazinare l'opzione letta da getopt

    //Loop principale per processare le opzione da CLI
    while ((opt = getopt(argc, argv, "n:m:o:h")) != -1){
        switch (opt){
            case '?': //Opzione non riconosciuta
                fprintf(stderr, "Errore: opzione non valida\n");
                return -1;
            default:

                //DEBUG
                // Otteniamo l'indice corretto per l'array V
                int index = opt_index(opt);
                if (index == -1) {
                    fprintf(stderr, "Errore: indice non valido per l'opzione -%c\n", opt);
                    return -1;
                }
                // Invoca la funzione corrispondente dall'array V
                if (V[index]((optarg == NULL ? argv[0] : optarg)) == -1) {
                    fprintf(stderr, "Errore nella gestione dell'opzione -%c\n", opt);
                    return -1;
                }
        }
    }
    return 0;
}
