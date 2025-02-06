#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "utils.h"

#define DEFAULT_BUFFER_SIZE 256 ///< Dimensione di buffer predefinita

/**
 * @brief Stampa l'uso corretto del programma e termina
 * @param progname Nome del programma
 */
void print_usage(const char *progname) {
    fprintf(stderr, "Uso: %s [sc|std] filein fileout [buffersize]\n", progname);
    fprintf(stderr, " sc -> Usa chiamate di sistema (read/write)\n");
    fprintf(stderr, " std -> Usa chiamate di libreria (fread/fwrite)\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief Copia un file utilizzando chiamate di sistema (read/write).
 * @param file_in Nome del file in input
 * @param file_out Nome del file in output
 * @param buffersize Dimensione del buffer
 */
void mycp_sc(const char *file_in, const char *file_out, size_t buffersize) {
    int fd_in, fd_out;
    char *buffer;
    ssize_t bytes_read, bytes_written;
    mode_t old_mask = umask(033); // Aggiunto per gestione permessi file

    // Apertura file di input in READ-ONLY
    SYSCALL("open", fd_in, open(file_in, O_RDONLY), "Errore apertura file di input %s: errno = %d\n", file_in, errno);

    // Creazione file di output con permessi standard
    SYSCALL("open", fd_out, open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644), "Errore apertura file di output %s: errno = %d\n", file_out, errno);

    umask(old_mask); // Ripristina la maschera originale

    // Allocazione buffer
    buffer = (char *)malloc(buffersize);
    if (!buffer) {
        perror("Errore allocazione buffer");
        close(fd_in);
        close(fd_out);
        exit(EXIT_FAILURE);
    }

    // Lettura e scrittura fino a EOF
    while ((bytes_read = read(fd_in, buffer, buffersize)) > 0) {
        SYSCALL("write", bytes_written, write(fd_out, buffer, bytes_read), "Errore scrittura file di output %s: errno = %d\n", file_out, errno);
    }

    if (bytes_read == -1) {
        perror("Errore in lettura");
    }

    free(buffer);
    SYSCALL("close", bytes_read, close(fd_in), "Errore chiusura file di input %s: errno = %d\n", file_in, errno);
    SYSCALL("close", bytes_read, close(fd_out), "Errore chiusura file di output %s: errno = %d\n", file_out, errno);
}

/**
 * @brief Copia un file utilizzando chiamate di libreria (fread/fwrite).
 * @param file_in Nome del file in input
 * @param file_out Nome del file in output
 * @param buffersize Dimensione del buffer
 */
void mycp_std(const char *file_in, const char *file_out, size_t buffersize) {
    FILE *fp_in, *fp_out;
    char *buffer;
    size_t bytes_read;
    mode_t old_mask = umask(033); // Aggiunto per coerenza con la versione system call

    // Apertura file con gestione errori tramite FOPEN
    FOPEN(fp_in, file_in, "rb");
    FOPEN(fp_out, file_out, "wb");

    umask(old_mask);

    // Allocazione del buffer
    buffer = (char *)malloc(buffersize);
    if (!buffer) {
        perror("Errore allocazione buffer");
        fclose(fp_in);
        fclose(fp_out);
        exit(EXIT_FAILURE);
    }

    // Lettura e scrittura fino a EOF
    while ((bytes_read = fread(buffer, 1, buffersize, fp_in)) > 0) {
        if (fwrite(buffer, 1, bytes_read, fp_out) != bytes_read) {
            perror("Errore in scrittura");
            print_errors("Errore scrittura file di output %s: errno = %d\n", file_out, errno);
            free(buffer);
            fclose(fp_in);
            fclose(fp_out);
            exit(EXIT_FAILURE);
        }
    }

    if (ferror(fp_in)) {
        perror("Errore in lettura");
    }

    free(buffer);
    fclose(fp_in);
    fflush(fp_out); // Aggiunto per garantire la scrittura dei buffer
    fclose(fp_out);
}

/**
 * @brief Funzione principale del programma
 * @param argc Numero di argomenti
 * @param argv Array di argomenti
 * @return 0 se l'operazione ha successo, EXIT_FAILURE in caso di errore
 */
int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        print_usage(argv[0]);
    }

    long buffer_size = DEFAULT_BUFFER_SIZE;
    if (argc == 5 && isNumber(argv[4], &buffer_size) != 0) {
        fprintf(stderr, "Dimensione buffer non valida\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "sc") == 0) {
        printf("--- Copia usando system calls... ---\n");
        mycp_sc(argv[2], argv[3], (size_t)buffer_size);
    } else if (strcmp(argv[1], "std") == 0) {
        printf("--- Copia usando chiamate di libreria... ---\n");
        mycp_std(argv[2], argv[3], (size_t)buffer_size);
    } else {
        print_usage(argv[0]);
    }
    return 0;
}
