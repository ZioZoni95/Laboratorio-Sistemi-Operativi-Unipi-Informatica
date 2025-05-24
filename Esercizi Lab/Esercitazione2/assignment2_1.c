/*
 * Esercizio 1: Utilizzo di getopt per il parsing della command line
 * 
 * Questo programma accetta quattro opzioni dalla riga di comando:
 * -n <num. intero>
 * -m <num. intero>
 * -o <stringa>
 * -h (help)
 *
 * L'opzione -h non ha argomenti e stampa un messaggio di aiuto.
 * Il parsing viene effettuato tramite getopt.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

void print_usage() {
    printf("Uso: nome-programma -n <num. intero> -m <num. intero> -o <stringa> -h per aiuto\n");
}

int main(int argc, char *argv[]){
    int opt;
    int n_value = 0, m_value = 0;
    char *o_value = NULL;

    //Parsing della riga di comando
    while((opt = getopt(argc,argv, "n:m:o:h")) != -1){
        switch (opt)
        {
        case 'n':
            n_value = atoi(optarg);
            printf("Opzione -n %d\n", n_value);
            break;
        
        case 'm': // Opzione -m con valore intero
                m_value = atoi(optarg);
                printf("Opzione -m: %d\n", m_value);
                break;
            case 'o': // Opzione -o con valore stringa
                o_value = optarg;
                printf("Opzione -o: %s\n", o_value);
                break;
            case 'h': // Opzione -h: stampa il messaggio di aiuto
                print_usage();
                return 0;
            case '?': // Caso di errore: opzione non riconosciuta
                fprintf(stderr, "Errore: opzione non valida\n");
                return -1;
        }
    }
    return 0;
}