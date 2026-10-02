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
    // Stampa l'uso del programma in caso di argomenti errati
    fprintf(stderr, "Uso: %s [sc|std] filein fileout [buffersize]\n", progname);
    fprintf(stderr, " sc -> Usa chiamate di sistema (read/write)\n");
    fprintf(stderr, " std -> Usa chiamate di libreria (fread/fwrite)\n");
    exit(EXIT_FAILURE); // Termina l'esecuzione in caso di errore
}

/**
 * @brief Copia un file utilizzando chiamate di sistema (read/write).
 * @param file_in Nome del file in input
 * @param file_out Nome del file in output
 * @param buffersize Dimensione del buffer
 */
void mycp_sc(const char *file_in, const char *file_out, size_t buffersize) {
    int fd_in, fd_out;            // Descrittori di file per input e output
    char *buffer;                 // Buffer per la lettura e scrittura
    ssize_t bytes_read, bytes_written; // Variabili per monitorare i byte letti e scritti
    mode_t old_mask = umask(033); // Imposta una maschera di permessi per il file

    // Apre il file di input in modalità sola lettura (O_RDONLY)
    SYSCALL("open", fd_in, open(file_in, O_RDONLY), "Errore apertura file di input %s: errno = %d\n", file_in, errno);

    // Apre o crea il file di output in modalità scrittura (O_WRONLY, O_CREAT, O_TRUNC)
    SYSCALL("open", fd_out, open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644), "Errore apertura file di output %s: errno = %d\n", file_out, errno);

    umask(old_mask); // Ripristina la maschera originale dei permessi

    // Alloca memoria per il buffer di lettura
    buffer = (char *)malloc(buffersize);
    if (!buffer) {
        perror("Errore allocazione buffer");
        close(fd_in); // Chiude il file di input in caso di errore
        close(fd_out); // Chiude il file di output in caso di errore
        exit(EXIT_FAILURE); // Termina il programma con errore
    }

    // Legge dal file di input e scrive nel file di output fino alla fine del file
    while ((bytes_read = read(fd_in, buffer, buffersize)) > 0) {
        SYSCALL("write", bytes_written, write(fd_out, buffer, bytes_read), "Errore scrittura file di output %s: errno = %d\n", file_out, errno);
    }

    if (bytes_read == -1) { // Se c'è un errore durante la lettura
        perror("Errore in lettura");
    }

    free(buffer); // Libera la memoria allocata per il buffer

    // Chiude i file dopo aver finito
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
    FILE *fp_in, *fp_out;    // Puntatori a FILE per i file di input e output
    char *buffer;             // Buffer per la lettura e scrittura
    size_t bytes_read;        // Variabile per i byte letti
    mode_t old_mask = umask(033); // Imposta una maschera di permessi per il file

    // Apre il file di input in modalità binaria (rb) e il file di output in modalità binaria (wb)
    FOPEN(fp_in, file_in, "rb");
    FOPEN(fp_out, file_out, "wb");

    umask(old_mask); // Ripristina la maschera originale dei permessi

    // Alloca memoria per il buffer di lettura
    buffer = (char *)malloc(buffersize);
    if (!buffer) {
        perror("Errore allocazione buffer");
        fclose(fp_in); // Chiude il file di input in caso di errore
        fclose(fp_out); // Chiude il file di output in caso di errore
        exit(EXIT_FAILURE); // Termina il programma con errore
    }

    // Legge e scrive dati finché non raggiunge la fine del file
    while ((bytes_read = fread(buffer, 1, buffersize, fp_in)) > 0) {
        if (fwrite(buffer, 1, bytes_read, fp_out) != bytes_read) {
            perror("Errore in scrittura");
            print_errors("Errore scrittura file di output %s: errno = %d\n", file_out, errno);
            free(buffer); // Libera la memoria allocata per il buffer
            fclose(fp_in); // Chiude il file di input in caso di errore
            fclose(fp_out); // Chiude il file di output in caso di errore
            exit(EXIT_FAILURE); // Termina il programma con errore
        }
    }

    if (ferror(fp_in)) { // Se c'è un errore durante la lettura
        perror("Errore in lettura");
    }

    free(buffer); // Libera la memoria allocata per il buffer
    fclose(fp_in); // Chiude il file di input
    fflush(fp_out); // Garantisce che tutti i dati vengano scritti su disco
    fclose(fp_out); // Chiude il file di output
}

/**
 * @brief Funzione principale del programma
 * @param argc Numero di argomenti
 * @param argv Array di argomenti
 * @return 0 se l'operazione ha successo, EXIT_FAILURE in caso di errore
 */
int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        // Se il numero di argomenti non è corretto, stampa l'uso del programma
        print_usage(argv[0]);
    }

    long buffer_size = DEFAULT_BUFFER_SIZE; // Imposta la dimensione di buffer predefinita
    if (argc == 5 && isNumber(argv[4], &buffer_size) != 0) { 
        // Se viene fornita una dimensione di buffer non valida, termina il programma
        fprintf(stderr, "Dimensione buffer non valida\n");
        exit(EXIT_FAILURE);
    }

    // Se il primo argomento è "sc", copia il file usando le system calls
    if (strcmp(argv[1], "sc") == 0) {
        printf("--- Copia usando system calls... ---\n");
        mycp_sc(argv[2], argv[3], (size_t)buffer_size);
    } 
    // Se il primo argomento è "std", copia il file usando le chiamate di libreria
    else if (strcmp(argv[1], "std") == 0) {
        printf("--- Copia usando chiamate di libreria... ---\n");
        mycp_std(argv[2], argv[3], (size_t)buffer_size);
    } 
    // Se il primo argomento non è valido, stampa l'uso del programma
    else {
        print_usage(argv[0]);
    }
    return 0; // Programma terminato correttamente
}
