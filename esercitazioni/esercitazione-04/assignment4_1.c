/*
Esercizio 1:
Scrivere un programma C che legge il file testuale "/etc/passwd" e scrive in un file testuale la lista dei 'login name' del sistema, uno per riga.
Il nome del file testuale di output viene passato come unico argomento al programma.
Il formato del file "/etc/passwd" è descritto nella sezione 5 del manuale entry 'passwd' (man 5 passwd):
"/etc/passwd contiene una riga per ogni account utente, con sette campi delimitati da due punti (":")."
Il primo campo è il 'login name'.
Suggerimento: usare fgets per leggere le righe del file; usare strchr per trovare la prima occorrenza di ':' nella stringa letta dal file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024 // Definiamo una lunghezza massima per le righe lette

int main(int argc, char *argv[]) {
    // Controlliamo che venga fornito un solo argomento (il file di output)
    if (argc != 2) {
        fprintf(stderr, "Uso corretto: %s <file_output>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Apriamo il file /etc/passwd in modalità lettura
    FILE *input_file = fopen("/etc/passwd", "r");
    if (input_file == NULL) {
        perror("Errore nell'apertura di /etc/passwd");
        return EXIT_FAILURE;
    }

    // Apriamo il file di output in modalità scrittura
    FILE *output_file = fopen(argv[1], "w");
    if (output_file == NULL) {
        perror("Errore nell'apertura del file di output");
        fclose(input_file);
        return EXIT_FAILURE;
    }

    char line[MAX_LINE_LENGTH]; // Buffer per la lettura di ogni riga
    
    // Leggiamo il file riga per riga
    while (fgets(line, sizeof(line), input_file) != NULL) {
        // Troviamo la prima occorrenza di ':' per delimitare il 'login name'
        char *colon_pos = strchr(line, ':');
        if (colon_pos != NULL) {
            *colon_pos = '\0'; // Sostituiamo ':' con terminatore di stringa per ottenere il login name
        }
        
        // Scriviamo il login name nel file di output
        fprintf(output_file, "%s\n", line);
    }

    // Chiudiamo i file
    fclose(input_file);
    fclose(output_file);

    printf("Lista dei login names scritta correttamente su %s\n", argv[1]);
    return EXIT_SUCCESS;
}