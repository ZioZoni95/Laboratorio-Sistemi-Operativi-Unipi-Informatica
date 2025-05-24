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

        if(strcmp(input, "exit") == 0){     //Comando per uscire dalla shell
            break;
        }

        //Tokenizzazione della riga di comando per ottenere i singoli argomenti
        int argc = 0;
        char *token = strtok(input, " ");
        while(token != NULL && argc < MAX_ARGS -1){
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL; //L'array degli argomenti deve essere terminato da NULL

        if(argc == 0) continue; //Se non ci sono comandi, ripeti il ciclo

        //Esempio di utilizzo della funzione isNumber per il primo argomento
        unsigned int number;
        if(isNumber(args[0], &number) == 0){
            printf("'%s' è un numero valido: %u\n", args[0], number);
            continue;
        }

        pid = fork(); //Creo un nuovo processo

        if(pid < 0){ //Errore della fork
            perror("Impossibile creare il processo figlio");
            continue;
        }

        if(pid == 0){ //Codice del processo figlio
            execvp(args[0], args); //Esegue il comando
            perror("Errore nell'esecuzione del comando"); //Se execvp ritorna c'è stato un errore
            exit(EXIT_FAILURE);
        }
        else{ //Codice del processo padre
            if(waitpid(pid, &status, 0) == -1){ //Attende la twerminazione del figlio
                perror("Errore nell'attesa del processo figlio");
            }
        }
    }

    return 0;
}