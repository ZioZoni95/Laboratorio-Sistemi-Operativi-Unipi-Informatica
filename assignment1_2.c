/*Esercizio 2
Scrivere una funzione con nome 'mystrcat' con il seguente prototipo:

char *mystrcat(char* buffer, size_t buffer_size, char *prima, ...);
La funzione prende come parametri un buffer, la lunghezza del buffer in bytes ed
almeno una stringa (il parametro formale 'prima'). Le stringhe possono essere in numero
variabile (>1). La funzione concatena tutte le stringhe passate come argomento all'interno del 
'buffer' e ritorna il buffer stesso. ATTENZIONE alla gestione della memoria!
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

const int REALLOC_INC = 16; // incremento della dimensione del buffer

/**
 * @brief Rialloca un buffer all nuova dimensione richiesta
 * 
 * @param buf Puntatore al buffer (passato per riferimento)
 * @param newsize Nuova dimensione del buffer
 */
void RIALLOCA(char** buf, size_t newsize){
    char* tmp = realloc(*buf, newsize);
    if(!tmp){ //check se realloc ha fallito
        perror("Errore di allocazione memoria");
        exit(1); // se fallisce termino il programma
    }
    *buf = tmp; //aggiorno il buffer con il nuovo indirizzo di memoria
}

/**
 * @brief Concatena un numero variabile di stringhe in un buffer dinamico
 * 
 * @param buf Buffer in cui concatenare le stringhe
 * @param sz Dimensione iniziale del buffer
 * @param first Prima stringa (necessaria)
 * @param ... Altre Stringhe da concatenare (terminate da NULL)
 * @return char* Puntatore al buffer contenente la stringa concatenata
 */
char *mystrcat(char *buf, size_t sz, char *first, ...){
    va_list args; //Dichiarazione di una lista di argomenti variabili
    va_start(args, first); //Inizializzazione della lista con il primo parametro

    size_t used = strlen(buf);
    size_t avaiable = sz - used; //spazio disponibile nel buffer

    char *current = first; //Puntatore alla prima stringa ricevuta
    while (current){ //Itero finchè argomento non è NULL
        size_t len = strlen(current); //Lunghezza della stringa corrente

        //Se lo spazio nel buffer non è sufficiente, riallochiamo la memoria
        if(len + used + 1 > sz){ //+1 è il terminatore di fine stringa
            sz = used + len + 1 + REALLOC_INC; //Aumento la dimensione
            RIALLOCA(&buf, sz); //Riallochiamo la memoria
            avaiable = sz - used; //Aggiorno lo spazio disponibile
        }

        strcat(buf, current); //Concatenazione della stringa nel buffer
        used += len; //Aggiorno il numero di caratteri usati
        avaiable -= len; //Aggirono lo spazio disponibile

        current = va_arg(args, char*); //Leggiamo la prossima stringa dalla lista di argomenti
    }

    va_end(args); //Chiudiamo la gestione della lista di argomenti variabili
    return buf; //Restituiamo il buffer aggiornato   
}

int main(int argc, char *argv[]){
    //controllo se sono stati bpassati almeno 6 argomenti
    if(argc < 7){
        printf("Troppi pochi argomenti\n");
        return -1;
    }

    char *buffer = NULL; //Dichiarazione del buffer
    RIALLOCA(&buffer, REALLOC_INC); //Alloca il buffer iniziale con una dimensione minima
    buffer[0] = '\0';

    //Concatenazione degli argomenti da CLI nel buffer
    buffer = mystrcat(buffer, REALLOC_INC, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], NULL);

    //Stampo il risultato finale
    printf("%s\n", buffer);

    free(buffer); //Liberiamo la memoria allocata per il buffer
    return 0;
}