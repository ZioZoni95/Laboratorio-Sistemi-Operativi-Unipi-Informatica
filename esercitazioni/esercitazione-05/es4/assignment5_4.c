/*
Esercizio 4:
Scrivere un programma C chiamato 'lsdir' che, dato come argomento un nome di directory, visita ricorsivamente tutto il sottoalbero di directory che ha come radice la directory passata come argomento. Per ogni directory, il programma deve stampare sullo standard output le informazioni sui file nel seguente formato:

Directory: <nomedir1>
file1     size    permessi
file2     size    permessi
------------------
Directory: <nomedir2>
file3     size    permessi
file4     size    permessi
------------------

Le directories '.' e '..' devono essere ignorate.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

//Funzione per stampare i permessi di un file in formato 'rwxr-xr-x'
void stampa_permessi(mode_t mode){
    char perms[10] = "---------";
    if(mode & S_IRUSR) perms[0] = 'r';  //Permessi di lettura per il proprietario
    if(mode & S_IWUSR) perms[1] = 'w';  // Permessi di scrittura per il proprietario
    if(mode & S_IXUSR) perms[2] = 'x';  //Permessi di esecuzione per il proprietario
    if(mode & S_IRGRP) perms[3] = 'r';  //Permessi di lettura per il gruppo
    if(mode & S_IWGRP) perms[4] = 'w';  //Permessi di scrittura per il gruppo
    if(mode & S_IXGRP) perms[5] = 'x';  //Permessi di esecuzione per il gruppo
    if(mode & S_IROTH) perms[6] = 'r';  //Permessi di lettura per gli altri utenti
    if(mode & S_IWOTH) perms[7] = 'w';  //Permessi di scrittura per gli altri utenti
    if(mode & S_IXOTH) perms[8] = 'x';  //Permessi di esecuzione per gli altri utenti
    printf("%s", perms);
}

//Funzione ricorsiva per visitare una directory e stampare le informazioni sui file
void list_directory(const char *dirpath){
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char filepath[1024];

    //Apre la directory, gestendo errori con la macro SYSCALL
    dir = opendir(dirpath);
    if (dir == NULL) {
        perror("Impossibile aprire la directory");
        return;
    }
    
    printf("Directory: %s\n", dirpath);
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;   //Ignoro le directory '.' e '..'
        }
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name); //Crea il percorso completo

        // Verifica se è un link simbolico che punta a una destinazione non valida
        if (lstat(filepath, &statbuf) == 0 && S_ISLNK(statbuf.st_mode)) {
            if (stat(filepath, &statbuf) != 0) { // Se il link simbolico è "dangling"
                printf("%-20s %10s  link invalido\n", entry->d_name, "N/A");
                continue;
            }
        }

        if (stat(filepath, &statbuf) == 0) {
            printf("%-20s %10ld ", entry->d_name, statbuf.st_size); //Stampa nome e dimensione del file
            stampa_permessi(statbuf.st_mode);   //Stampa i permessi del file
            printf("\n");
        } else {
            printf("Impossibile ottenere informazioni sul file %s\n", filepath);
        }
    }
    printf("------------------\n");

    //Ricorsione sulle sottodirectory
    rewinddir(dir); // Torna all'inizio della directory per ricominciare la lettura
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
        if (stat(filepath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            list_directory(filepath); //Chiamata ricorsiva per le sottodirectory
        }
    }
    closedir(dir);  
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    list_directory(argv[1]);    //Avvia la scansione della directory;
    return 0;
}
