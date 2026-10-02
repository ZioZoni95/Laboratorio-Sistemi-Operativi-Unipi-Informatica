/**
 * @file assignment5_1.c
 * @brief Implementazione di un programma equivalente al comando UNIX "cp"
 * 
 * Esercitazione 5 - Esercizio 1
 * Scrivere un programma, utilizzando chiamate di sistema, che implementi
 * l'equivalente del comando UNIX cp. Ilprogramma devve accettare 3 argomenti:
 * 
 *      mycp filein fileout [buffersize]
 * 
 * L'argomento 'buffersize' è la dimensione del buffer da utilizzare per le
 * letture e scritture con le SCs "read" e "write" (se non specificato assegnare
 * un valore di default, es 256 bite). Realizzare quindi lo stesso programma,
 * utilizando le chiamate di libreria "fread" e "fwrite". Chiamiamo questa seconda 
 * versione 'mycp_std'.
 * 
 * Confrontare le presrtazioni (usando il comando time da CLI) al variare del
 * parametro 'buffersize'
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEFAULT_BUFFER_SIZE 256 ///< Dimensione di buffer predefinita

/**
 * @brief Stampa l'uso correto del programma e termina
 * @param progname Nome del programma
 */
void print_usage(const char *progname){
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
void mycp_sc(const char *file_in, const char *file_out, size_t buffersize){
    int fd_in, fd_out;
    char *buffer;
    ssize_t bytes_read, bytes_written;

    //Apertura file di input in READ-ONLY
    fd_in = open(file_in, O_RDONLY);
    if(fd_in == -1){
        perror("Errore apertura file di input");
        exit(EXIT_FAILURE);
    }

    //Creazione file di output con permessi standard
    fd_out = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd_out == -1){
        perror("Errore apertura file di output");
        exit(EXIT_FAILURE);
    }

    //Allocazione buffer
    buffer = (char *)malloc(buffersize);
    if(!buffer){
        perror("Errore allocazione buffer");
        close(fd_in);
        close(fd_out);
        exit(EXIT_FAILURE);
    }

    //Lettura e scrittura fino a EOF
    while((bytes_read = read(fd_in, buffer, buffersize)) > 0){
        bytes_written = write(fd_out, buffer, bytes_read);
        if(bytes_written != bytes_read){
            perror("Errore in scrittura");
            free(buffer);
            close(fd_in);
            close(fd_out);
            exit(EXIT_FAILURE);
        }
    }

    if(bytes_read == -1) perror("Errore in lettura");

    free(buffer);
    close(fd_in);
    close(fd_out);
}

/**
 * @brief Copia un file utilizzando chiamate di libreria (fread/fwrite).
 * @param file_in Nome del file in input
 * @param file_out Nome del file in output
 * @param buffersize Dimensione del buffer
 */
void mycp_std(const char *file_in, const char* file_out, size_t buffersize){
    FILE *fp_in, *fp_out;
    char *buffer;
    size_t bytes_read;

    //Apertura file in modalità BINARY
    fp_in = fopen(file_in, "rb");
    if(!fp_in){
        perror("Errore apertura file di input");
        exit(EXIT_FAILURE);
    }

    fp_out = fopen(file_out, "wb");
    if(!fp_out){
        perror("Errore apertura file di output");
        fclose(fp_in);
        fclose(fp_out);
        exit(EXIT_FAILURE);
    }

    //Allocazione del buffer
    buffer = (char*)malloc(buffersize);
    if(!buffer){
        perror("Errore di allocazione buffer");
        fclose(fp_in);
        fclose(fp_out);
        exit(EXIT_FAILURE);
    }

    //Lettura e scrittura fino a EOF
    while((bytes_read = fread(buffer, 1, buffersize, fp_in)) > 0){
        if(fwrite(buffer, 1, bytes_read, fp_out) != bytes_read){
            perror("Errore in scrittura");
            free(buffer);
            fclose(fp_in);
            fclose(fp_out);
            exit(EXIT_FAILURE);
        }
    }

    if(ferror(fp_in)) perror("Errore in lettura");

    free(buffer);
    fclose(fp_in);
    fclose(fp_out);
}

/**
 * @brief Funzione principale del programma
 * @param argc Numero di argomenti
 * @param argv Array di argomenti
 * @return 0 se l'operazione ha successo, EXIT _FAILURE in caso di errore
 */
int main(int argc, char *argv[]){
    if(argc < 4 || argc > 5){
        print_usage(argv[0]);
    }

    size_t buffer_size = (argc == 5) ? atoi(argv[4]) : DEFAULT_BUFFER_SIZE;
    if(buffer_size <= 0){
        fprintf(stderr, "Dimensione buffer non valida\n");
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[1], "sc") == 0){
        printf("---Copia usando system calls...---\n");
        mycp_sc(argv[2], argv[3], buffer_size);
    }else if(strcmp(argv[1], "std") == 0){
        printf("---Copia usando chiamate di libreria...---\n");
        mycp_std(argv[2], argv[3], buffer_size);
    }else{
        print_usage(argv[0]);
    }
    return 0;
}