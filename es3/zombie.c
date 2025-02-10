/**Esercizio 3
*Un processo zombie è un processo terminato ma che
*ancora possiede delle risorse ('pid' e PCB) che non possono 
*essere liberate perché il processo padre, non ancora terminato, 
*potrebbe decidere di leggere lo exit status del processo figlio.
*Scrivere un programma che prende un intero N come argomento e
*crea N processi zombies. Lanciare il programma in background 
*e visualizzare gli zombies con il comando bash ps -A -ostat,pid,ppid | grep Z.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "utils.h"

int main(int argc, char *argv[]){
    //Controllo degli argomenti
    if(argc != 2){
        fprintf(stderr, "Uso: %s <numero_processi_zombie>\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned int N;
    if(isNumber(argv[1], &N) != 0){
        fprintf(stderr, "Errore: L'argomento deve essere un numero intero positivo.\n");
        return EXIT_FAILURE;
    } 

    printf("Creazione di %u processi zombie...\n", N);

    for(unsigned int i = 0; i < N; i++){
        pid_t pid = fork();

        if(pid < 0){
            perror("fork fallita");
            return EXIT_FAILURE;
        }

        if(pid == 0){
            //Figlio: termina subito creando uno zombie
            printf("Figlio %d creato (PID: %d)\n", i + 1, getpid());
            exit(0);
        }
    }

    //Il padre rimane in esecuzione e NON chiama wait(), quindi i figli rimangono zombie
    printf("Processi zombie creati! | Controllarli con:\n");
    printf(" ps -A -ostat,pid,ppid | grep Z\n");

    //Il padre aspetta l'input prima di terminare, per lasciare gli zombie visibili
    sleep(30);

    printf("Il processo padre sta terminando, gli zombie verranno rimossi...\n");
    return EXIT_SUCCESS;
}