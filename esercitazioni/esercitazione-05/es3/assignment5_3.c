/*
Esercizio 3 (permessi.c)
Scrivere un programma C che dati come argomenti una lista di file o directories stampa per ogni argomento:
- il nome del file/directory,
- il numero dell'inodo associato,
- il tipo di file (-,l,d,s,…),
- i bit di protezione (es. rw-r--r--),
- l'user identifier (uid),
- il group identifier (gid),
- la size del file,
- il timestamp dell'ultima modifica.
Per convertire il tempo di ultima modifica in un formato stampabile usare la funzione di libreria ctime.
Usare getpwuid e getgrgid per convertire uid e gid nei nomi corrispondenti agli id.
*/

#define _POSIX_C_SOURCE  200112L  // Necessario per S_ISSOCK

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "utils.h"

/**
 * @brief Stampa i permessi e le informazioni di un file/directory
 * @param filename Nome del file o dirctory
 */
void permessi(const char filename[]){
    struct stat statbuf;
    int r;

    //Eseguo la syscall stat e gestisco enventuali errori con la macro
    SYSCALL(stat, r, stat(filename, &statbuf),"Facendo stat del nome %s: errno=%d\n",filename,errno);
    
    //Inizializzazione della stringa dei permessi
    char mode[11] = {'-','-','-','-','-','-','-','-','-','-','\0'};

    //Determino il tipo di file
    if(S_ISDIR(statbuf.st_mode)) mode[0] = 'd'; //Directory
    if(S_ISCHR(statbuf.st_mode)) mode[0] = 'c'; //File a caratteri
    if(S_ISBLK(statbuf.st_mode)) mode[0] = 'b'; //File a blocchi
    if(S_ISLNK(statbuf.st_mode)) mode[0] = "l"; //Link simbolico
    if(S_ISFIFO(statbuf.st_mode)) mode[0] = 'p'; //FIFO/Named Pipe
    if(S_ISSOCK(statbuf.st_mode)) mode [0] = 's'; //socket

    //Determiniamo i permessi utente, gruppo e altri
    if(S_IRUSR & statbuf.st_mode) mode[1] = 'r';
    if(S_IWUSR & statbuf.st_mode) mode[2] = 'w';
    if(S_IXUSR & statbuf.st_mode) mode[3] = 'x';
    if(S_IRGRP & statbuf.st_mode) mode[4] = 'r';
    if(S_IWGRP & statbuf.st_mode) mode[5] = 'w';
    if(S_IXGRP & statbuf.st_mode) mode[6] = 'x';
    if(S_IROTH & statbuf.st_mode) mode[7] = 'r';
    if(S_IWOTH & statbuf.st_mode) mode[8] = 'w';
    if(S_IXOTH & statbuf.st_mode) mode[9] = 'x';

    //ottengo i nomi utente e gruppo a partire da UID e GID
    struct passwd *pw = getpwuid(statbuf.st_uid);
    struct group *gr = getgrgid(statbuf.st_gid);

    //stampo le informazioni del file
    fprintf(stdout, "%-20s (%-7u): %-10ld %-s %-s,%-s %-s", 
            filename,
            (unsigned)statbuf.st_ino,   //numero di i-node
            statbuf.st_size,            //Dimensione del file
            mode,                       //Stringa permessi
            pw->pw_name,                //Nome utente
            gr->gr_name,                //Nome gruppo
            ctime(&statbuf.st_mtime));  //Timestamp ultima modifica
}

/**
 * @brief Funzione principale
 * @param argc Numero di argomenti
 * @param argv Lista di argomenti (nomi di file o directory)
 * @return EXIT_SUCCESS se tutto ok, EXIT_FAILURE in caso di errore
 */
int main(int argc, char *argv[]){
    if(argc == 1){
        fprintf(stderr,"Us: %s file|directory [file|directory]...\n",argv[0]);
        return EXIT_FAILURE;
    }

    //Elaboro tutti i file e directory passati come argomenti
    for(int i = 1; i < argc; i++){
        permessi(argv[i]);
    }
    return EXIT_SUCCESS;
}