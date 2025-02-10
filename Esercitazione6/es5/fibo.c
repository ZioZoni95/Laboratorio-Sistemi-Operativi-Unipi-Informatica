/*
 * Esercizio 5
 * Completare il codice seguente in modo che il programma calcoli i primi 13 numeri di Fibonacci
 * utilizzando per ogni chiamata doFib un processo distinto. La funzione doFib ritorna al processo
 * padre il valore calcolato tramite l'exit status (exit).
 * Ogni processo deve eseguire doFib al massimo una volta.
 * Se l'argomento doPrint e' 1, la funzione stampa il numero calcolato prima di passarlo al processo padre.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/**
 * Calcola ricorsivamente il numero di Fibonacci dell'argomento 'n'.
 * Ogni chiamata ricorsiva avviene in nuovo processo figlio
 */
static void doFib(int n, int doPrint){
    if (n == 0){
        if(doPrint) printf("%d\n", 0);
        exit(0);
    } 
    if (n == 1){
        if(doPrint) printf("%d\n", 1);
        exit(1);
    }

    pid_t pid1, pid2;
    int status1, status2;

    //Creazione del primo processo figlio per calcolare doFib(n-1)
    pid1 = fork();
    if(pid1 < 0){
        perror("Error nella fork()");
        exit(EXIT_FAILURE);
    }
    if(pid1 == 0){
        doFib(n - 1, 0); //Il figlio calcola doFib(n-1) senza stampare
    }

    //Creazione del secondo processo figlio per calcolare doFib(n-2)
    pid2 = fork();
    if(pid2 < 0){
        perror("Errore nella fork()");
        exit(EXIT_FAILURE);
    }
    if(pid2 == 0){
        doFib(n - 2, 0); //Il figlio calcola doFib(n-2) senza stampare
    }

    //Il processo padre attende entrambi i figli
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    //Recupera i risultati dai figli
    int result1 = WEXITSTATUS(status1);
    int result2 = WEXITSTATUS(status2);
    int result = result1 + result2;

    //Stampa il risultato se richiesto
    if(doPrint){
        printf("%d\n", result);
    }

    //Ritorna il risultato al processo padre tramite l'exit status
    exit(result);
}

int main(int argc, char *argv[]){
    //Questo programma può calcolare i numeri di Fibonacci fino a 13
    const int NMAX = 13;
    int arg;

    //Controllo degli argomenti passati:
    if(argc != 2){
        fprintf(stderr, "Uso: %s <num>\n", argv[0]);
        return EXIT_FAILURE;
    }

    arg = atoi(argv[1]);
    if(arg < 0 || arg > NMAX){
        fprintf(stderr,"num deve essere compreso tra 0 e 13\n");
        return EXIT_FAILURE;
    }

    doFib(arg, 1);
    return 0;
}