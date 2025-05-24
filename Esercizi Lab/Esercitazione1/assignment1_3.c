/**
 * Esercizio 3
Non utilizzando la funzioni di libreria 'getopt' (vedere man 3 getopt), scrivere un programma che effettua il parsing della linea di comando e che riconosce le seguenti opzioni:

-n <numero> -s <stringa> -m <altro-numero> -h. 
Il programma dovrà stampare le opzioni riconosciute con il relativo argomento. L'opzione -h non ha argomento e corrisponde al messaggio di help (i.e. usage). Se è presente
l'opzione -h dovrà essere stampato solo il messaggio di di help cioè:

nome-programma -n <numero> -s <stringa> -m <numero> -h
Se ci sono opzioni non riconosciute queste dovranno essere 
stampate a video con il messaggio “opzione X non riconosciuta”. Per convertire le stringhe in interi usare la funzione di libreria la funzione strtol (vedere man 3 strtol)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Funzzione per verificare se una stringa rappresenta un numero valido
long isNumber(const char *s){
    char *e = NULL;
    long val = strtol (s, &e, 0);
    //se "e" punta al terminatore della stringa ==> conversione riuscita
    if(e != NULL && *e == '\0') return val;
    return -1;
}

//Funzione per stampare il messaggio di help
void print_help(const char *progname){
    printf("Usage: %s -n <numero> -s <stringa> -m <numero> -h per aiuto\n", progname);
}

int main(int argc, char *argv[]){
    //Se non ci sono argomenti sufficienti, mostra l'help e termina
    if(argc < 2){
        print_help(argv[0]);
        return 1;
    }

    int i = 1; //Indice per scorrere gli argomenti della CLI
    while(i < argc){
        if(strcmp(argv[i], "-h") == 0){
            //se presente -h, stampa l'help e termino il programma
            print_help(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "-m") == 0){
            //controllo se l'opzione "-n" o "-m" hanno argomenti validi
            if(i + 1 < argc && isNumber(argv[i + 1]) != -1){
                printf("%s: %s\n", argv[i], argv[i + 1]);
                i += 2; //prossima opzione
            }
            else {
                printf("Errore: L'opzione %s richiede un numero valido di argomenti\n", argv[i]);
                return 1;
            }
        }
        else if (strcmp(argv[i], "-s") == 0){
            //controllo se "-s" ha argomenti validi
            if(i + 1 < argc){
                printf("%s: %s\n",argv[i], argv[i + 1]);
                i += 2;
            }
            else{
                printf("Errore: L'opzione %s richiede una stringa\n", argv[i]);
                return 1;
            }
        }
        else{
            //Se l'opzione non è riconosciuta, stampo errore
            printf("Opzione %s non riconosciuta\n", argv[i]);
            i++;
        }
    }
    return 0;
}