/*
 * Esercizio 4
 * Scrivere un programma C che prende come argomento un intero N>1. 
 * L'intero N rappresenta il numero di processi figli che devono essere creati in totale.
 * I processi vanno creati come segue:
 * - Il processo main stampa il suo pid e crea un figlio attendendone quindi la sua terminazione.
 * - Il processo figlio a sua volta stampa il suo pid e crea un altro figlio attendendone la terminazione.
 * - Questo processo si ripete fino a ottenere N processi figli in totale.
 * 
 * L'output formattato che si richiede che venga stampato sullo standard output è il seguente:
 *
 * ./family 4
 * ---- 20894: creo un processo figlio
 * --- 20895: creo un processo figlio
 * -- 20896: creo un processo figlio
 * - 20897: creo un processo figlio
 * 20898: sono l'ultimo discendente
 * 20898: terminato con successo
 * - 20897: terminato con successo
 * -- 20896: terminato con successo
 * --- 20895: terminato con successo
 * ---- 20894: terminato con successo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "utils.h"

int main(int argc, char *argv[]){
    //Controllo degli argomenti passati
    if(argc != 2){
        fprintf(stderr, "Uso: %s <num_processi>\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned int N;
    //Verifica che l'argomento sia un numero e che sia maggiore di 1
    if(isNumber(argv[1], &N) != 0 || N <= 1){
        fprintf(stderr, "Errore: L'argomento deve essere un numero intero > 1.\n");
        return EXIT_FAILURE;
    }

    pid_t pid;
    int i;

    //Creazione dei processi figli in sequenza
    for(i = N; i > 0; i--){
        //Stampa con formattazione che aggiunge i "-" per ogni livello di profondità
        //La variabile 'i' determina la profondità dell'albero di processi
        printf("%*s%d: creo un processo figlio\n", N - i + 1, "-", getpid());
        fflush(stdout); //Forza la stampa immediata del messaggio

        pid = fork(); //Creazione del processo figlio

        if(pid < 0){
            //Messaggio di errore se la fork() fallisce
            perror("Errore nella fork()");
            exit(EXIT_FAILURE);
        }

        if(pid == 0){
            //Se siamo nel processo figlio, continuiamo a creare altri figli
            continue;
        }
        else{
            //Se siamo nel processo padre, attendiamo che il figlio termini
            waitpid(pid, NULL, 0);
            //Stampa che il figlio ha terminato
            printf("%*s%d: terminato con successo\n", N - i + 1, "-", getpid());
            fflush(stdout);
            //Una volta che il padre ha atteso il figlio, non è necessario continuare a creare nuovi figli
            break; //Esco dal ciclo, il padre non deve creawre altri figli
        }
    }

    //Caso speciale: Ultimo figlio della catena
    if(pid == 0){
        //L'ultimo figlio stampa il messaggio che è l'ultimo discendente
        printf("%d: sono l'ultimo discendente\n", getpid());
        fflush(stdout);
        exit(EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}
