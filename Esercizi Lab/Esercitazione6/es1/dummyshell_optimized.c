#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_INPUT 1024    // Lunghezza massima della riga di comando
#define MAX_ARGS 64       // Numero massimo di argomenti
#define MAX_STRING 64     // Lunghezza massima stringa input
#define MAX_PARAMS 256    // Numero massimo di parametri

// Funzione per allocare memoria
#define CHAR_MALLOC(ptr, size) \
    if ((ptr = (char*) malloc(sizeof(char) * size)) == NULL) { \
        perror("malloc"); \
        exit(EXIT_FAILURE); \
    }

// Funzione per stampare il prompt
#define print_dummyshell() \
    do { \
        fprintf(stdout, "dummyshellZioZoni> "); \
        fflush(stdout); \
    } while (0);

char read_command_line(char*, char**);
void free_argv(char**);
char is_exit_command(char*);

int main() {
    char input[MAX_INPUT];        // Buffer per la riga di comando
    char *args[MAX_ARGS];         // Array di puntatori per gli argomenti
    pid_t pid;                    // ID processo per fork
    int status;                   // Stato di terminazione del processo figlio

    while (1) {                   // Loop principale della shell
        print_dummyshell();       // Stampa il prompt

        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("Errore nella lettura dell'input");
            continue;
        }

        input[strcspn(input, "\n")] = '\0'; // Rimuove il carattere di new line

        if (is_exit_command(input)) { // Comando per uscire dalla shell
            break;
        }

        // Tokenizzazione della riga di comando
        int argc = 0;
        char *token = strtok(input, " ");
        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL; // Termina l'array degli argomenti con NULL

        if (argc == 0) continue; // Se non ci sono comandi, ripeti il ciclo

        // Creazione del processo figlio
        pid = fork();
        if (pid < 0) { // Errore della fork
            perror("Impossibile creare il processo figlio");
            continue;
        }

        if (pid == 0) { // Codice del processo figlio
            execvp(args[0], args); // Esegui il comando
            perror("Errore nell'esecuzione del comando"); // Se execvp ritorna, c'è stato un errore
            exit(EXIT_FAILURE);
        } else { // Codice del processo padre
            if (waitpid(pid, &status, 0) == -1) { // Attende la terminazione del processo figlio
                perror("Errore nell'attesa del processo figlio");
            }
        }
    }

    return 0;
}

// Funzione per verificare se il comando è 'exit'
char is_exit_command(char* command) {
    if (strncmp(command, "exit", 4) == 0) return 1;
    return 0;
}

// Funzione per leggere la riga di comando e separare gli argomenti
char read_command_line(char* input_line, char** argv) {
    if (fgets(input_line, MAX_STRING, stdin) == NULL) {
        perror("fgets");
        return -1;
    }

    char* tmp;
    char* token = strtok_r(input_line, " ", &tmp);
    size_t i = 0;
    size_t len = strlen(token) + 1;
    CHAR_MALLOC(argv[i], len);
    strncpy(argv[i], token, len);

    while ((token = strtok_r(NULL, " ", &tmp)) != NULL) {
        if (token[0] != '\0' && token[0] != '\n') {
            i++;
            len = strlen(token) + 1;
            CHAR_MALLOC(argv[i], len);
            token[strcspn(token, "\n")] = '\0'; // Rimuovo '\n'
            strncpy(argv[i], token, len);
        } else continue;
    }

    if (i == 0) return 1; // Non ci sono comandi da eseguire
    argv[i + 1] = NULL;
    return 0; // Successo
}

// Funzione per liberare la memoria allocata per gli argomenti
void free_argv(char** argv) {
    size_t i = 0;
    while (argv[i] != NULL) {
        free(argv[i]);
        i++;
    }
    free(argv);
    return;
}
