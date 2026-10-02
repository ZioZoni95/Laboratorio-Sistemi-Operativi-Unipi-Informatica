/*
Esercizio 2
Realizzare il comando myfind ispirato al comando di shell find

myfind dir nomefile

Il programma deve:
- Cercare ricorsivamente il file "nomefile" nel sottoalbero radicato nella directory "dir".
- Per ogni file "nomefile" trovato, stampare il path assoluto della directory in cui è stato trovato e la data dell'ultima modifica.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <linux/limits.h>
#include "utils.h"

//Funzione ricorsiva per cercare ul file target all'interno della directory specificata
void search_file(const char *dir_path, const char *target_file){
    DIR *directory;             //Puntatore alla directory
    struct dirent *entry;       //Struttura per leggere le entry della direcotry
    struct stat file_stat;      //Struttura per ottenere informazioni sul file
    char path[PATH_MAX];

    //Apertura della directory usando il wrapper SYSCALL
    SYSCALL(opendir, directory, opendir(dir_path), "Impossibile aprire la director: %s\n", dir_path);

    //Lettura delle entry della directory una per una
    while((errno = 0, entry = readdir(directory)) != NULL){
        //Ignora le special directories "." e ".." per evitare loop infiniti
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        //Costruzione del percorso completo del file/direcotry completo
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        //Ottengo le informazioni usando SYSCALL
        int r;
        SYSCALL(stat, r, stat(path, &file_stat), "Errore nel recupero delle informazioni del file: %s\n",path);

        //Se il nome del file corrisponde a quello cercato
        if(strcmp(entry->d_name, target_file) == 0){
            char abs_path[PATH_MAX];
            char *realpth;
            SYSCALL(realpath, realpth, realpath(path,abs_path), "Errore nel recupero del percorso assoluto\n",path);
            if(realpth == NULL) continue;

           //stampa il percorso assuluto e òa data daòò'ultima modifica
           printf("%s\t%s", abs_path, ctime(&file_stat.st_atime));
        }

        //Se l'entry è una directory esegue una ricerca ricorsiva
        if(S_ISDIR(file_stat.st_mode)){
            search_file(path, target_file); //Chiamata ricorsiva per esplorare le sottodirectory
        }
    }

    //Gestione degli errore della funzione readdir
    if(errno != 0){
        perror("readdir"); //Segnala eventuali errori di lettura della directory
    }

    //Chiusura della directory per liberare le risorse usando SYSCALL
    int r;
    SYSCALL(closedir, r, closedir(directory),"Errore nella chiusura della directory: %s\n",dir_path);
}

int main(int argc, char *argv[]){
    //verifica del numero corretto di argomenti passati
    if(argc != 3){
        fprintf(stderr, "Uso: %s <directory> <nomefile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Avvio della ricerca
    search_file(argv[1], argv[2]);
    return 0;
}