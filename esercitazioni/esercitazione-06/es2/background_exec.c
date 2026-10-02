/**
 * Esercizio 2
 *Scrivere un programma C che appena attivato va immediatamente in background,
 *attende per X secondi (eseguendo il programma /bin/sleep con una chiamata ad una exec*)
 *dove X e' l'argomento del programma e poi stampa il 
 *suo pid, il pid del padre e quindi termina.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/types.h> //Inclusione della libreria per i tipi di dati dei processi
 #include <sys/stat.h>  //Inclusione della libreria per la gestione dei permessi
 #include <fcntl.h>     //Inclusione della libreria per la destione dei file descriptor
 #include "utils.h"

 int main(int argc, char *argv[]){
    //Controllo degli argomenti passati
    if(argc != 2){
        fprintf(stderr,"Uso: %s <secondi>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    unsigned int seconds;
    if(isNumber(argv[1], &seconds) != 0){
        fprintf(stderr, "Errore: l'argomento deve essere un numero intero positivo.\n");
        exit(EXIT_FAILURE);
    }

    //Creazione del processo figlio
    pid_t pid = fork();
    if(pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid > 0){    //IL processo padre termina immediatamente
        exit(EXIT_SUCCESS);
    }

    //Il codice viene eseguito solo dal figlio

    //Creazione di una nuova sessione per dissociare il processo dal terminale
    if(setsid() < 0){
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    //Reindirizzamento di stdin, stdout e stderr a /dev/null per evitare output indesiderati
    int file_descriptor = open("/dev/null", O_RDWR);
    if(file_descriptor < 0){
        perror("open");
        exit(EXIT_FAILURE);
    }
    dup2(file_descriptor, STDIN_FILENO);
    dup2(file_descriptor, STDOUT_FILENO);
    dup2(file_descriptor, STDERR_FILENO);
    if(file_descriptor > 2) close(file_descriptor);

    //Esecuzione del programma /bin/sleep con il tempo specificato
    execl("/bin/sleep", "sleep", argv[1], (char*)NULL);

    //Se execl ritorna, signidica che c'è stato un errore
    perror("execl");
    exit(EXIT_FAILURE);
 }