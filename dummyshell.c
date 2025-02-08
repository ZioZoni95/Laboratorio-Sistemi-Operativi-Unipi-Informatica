/**
 * Esercizio1: dummyshell
 * Realizzare una shell rudimentale (dummyshell) che legge un comando 
 * con eventuali parametri dallo standard input e ne 
 * invoca l'esecuzione utilizzando una funzione di libreria
 * della famiglia exec*. La shell deve terminare se viene
 * digitato il comando 'exit'. Il formato dei comandi accettati dalla 
 * shell e' molto semplice e non non prevede metacaratteri, 
 * redirezione, pipe, lettura 
 * di variabili d'ambiente, etc…
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "utils.h"

#define MAX_INPUT 1024 //Lunghezza massima della riga di comando
#define MAX_ARGS 64

int main(){
    char input[MAX_INPUT]; //Buffer per la riga di comando
    char *args[MAX_ARGS]; //Array di puntatori per gli argomenti
    pid_t pid;            //ID processo per fork
    int status;           //Stato di terminazione del processo figlio

    while(1){             //Loop principale della shell
        printf("dummyshellZioZoni> "); //prompt per l'utente
        fflush(stdout);   //Assicura che il prompt sia visualizzato subito

        if(fgets(input, sizeof(input), stdin) == NULL){
            perror("Errore nella lettura dell'input");
            continue;
        }

        input[strcspn(input, "\n")] = '\0'; //rimuove il carattere di new line
    }
}