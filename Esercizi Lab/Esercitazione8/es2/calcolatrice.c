#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include "utilities.h"

#define MAX_LEN 1024 // Lunghezza massima dell'operazione
#define OPERAZIONE "Operazione: "
#define RISULTATO "Risultato: "

// Funzione per gestire le pipe
void close_pipefd(int* pipefd) {
    close(pipefd[0]);
    close(pipefd[1]);
}

// Funzione per eseguire l'operazione
void esegui_op(const char *operazione) {
    int pipefd_in[2];  // Pipe per comunicazione padre -> figlio (input)
    int pipefd_out[2]; // Pipe per comunicazione figlio -> padre (output)
    pid_t pid;
    char buffer[MAX_LEN];
    int err_in, err_out, err_dup_in, err_dup_out;

    // Creazione delle pipe con gestione degli errori tramite macro
    err_in = pipe(pipefd_in);
    EXIT_IF_EXPECTED_VALUE(err_in, -1, err_in, "pipe");

    err_out = pipe(pipefd_out);
    EXIT_IF_EXPECTED_VALUE(err_out, -1, err_out, "pipe");

    pid = fork();
    EXIT_IF_EXPECTED_VALUE(pid, -1, pid, "fork");

    if (pid == 0) { // Processo figlio
        close(pipefd_in[1]);  // Chiudo la scrittura della pipe di input
        close(pipefd_out[0]); // Chiudo la lettura della pipe di output

        // Redirigo stdin dalla pipe in ingresso e stdout alla pipe in uscita
        err_dup_in = dup2(pipefd_in[0], STDIN_FILENO);
        EXIT_IF_EXPECTED_VALUE(err_dup_in, -1, err_dup_in, "dup2");  // Redirigo stdin sulla pipe in ingresso

        err_dup_out = dup2(pipefd_out[1], STDOUT_FILENO);
        EXIT_IF_EXPECTED_VALUE(err_dup_out, -1, err_dup_out, "dup2"); // Redirigo stdout sulla pipe in uscita

        // Chiudo i descrittori di pipe non necessari
        close(pipefd_in[0]);
        close(pipefd_out[1]);

        // Disabilito il buffering di stdout nel figlio
        setbuf(stdout, NULL);

        // Eseguo il comando bc
        execlp("bc", "bc", "-lq", (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else { // Processo padre
        close(pipefd_in[0]);  // Chiudo la lettura della pipe di input
        close(pipefd_out[1]); // Chiudo la scrittura della pipe di output

        // Scrivo l'operazione nella pipe (comunicazione padre -> figlio)
        write(pipefd_in[1], operazione, strlen(operazione));
        write(pipefd_in[1], "\n", 1); // Aggiungo newline per terminare il comando
        close(pipefd_in[1]); // Chiudo la scrittura della pipe di input

        // Stampa l'operazione una sola volta
        printf("%s%s\n", OPERAZIONE, operazione);
        fflush(stdout); // Forza il flush del buffer in modo che l'operazione venga stampata subito

        // Leggo il risultato da `bc` dalla pipe di uscita e lo stampo
        ssize_t nread = read(pipefd_out[0], buffer, sizeof(buffer) - 1);
        if (nread > 0) {
            buffer[nread] = '\0'; // Terminatore stringa
            printf("%s%s", RISULTATO, buffer); // Stampo solo il risultato
            fflush(stdout); // Forza il flush del buffer del risultato
        }

        close(pipefd_out[0]); // Chiudo la lettura della pipe di output
        waitpid(pid, NULL, 0); // Attende il figlio
    }
}


// Funzione principale per gestire l'input dell'utente
int main(void) {
    char *operazione = NULL;
    const char exit_cmd[] = "quit";
    size_t len = 0;

    // Allocazione del buffer per l'operazione
    EXIT_IF_NULL(operazione, (char*) malloc(MAX_LEN * sizeof(char)), "malloc");

    while (1) {
        printf(OPERAZIONE);
        fflush(stdout); // Forza il flush per evitare buffering prima che l'utente scriva

        // Ottengo l'operazione dall'utente
        if (getline(&operazione, &len, stdin) == -1) {
            break;
        }

        // Rimuovo il carattere di newline se presente
        operazione[strcspn(operazione, "\n")] = '\0';

        // Controllo se l'utente vuole uscire
        if (strcmp(operazione, exit_cmd) == 0) {
            break;
        }

        // Eseguo l'operazione
        esegui_op(operazione);
    }

    // Deallocazione della memoria
    free(operazione);
    return 0;
}
