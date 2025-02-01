/*Esercizio 1
Scrivere una funzione C che prende in input come primo argomento una stringa,
come secondo argomento la lunghezza della stringa e restituisca nel terzo argomento
la stessa stringa con tutti i sui caratteri in maiuscolo:

void strtoupper(const char* in, size_t len, char* out);

Scrivere il programma main di test per la funzione 'strtoupper' che prende 
la/le stringa/e da passare alla funzione come argomenti da linea di comando.
 Per convertire una lettera in maiuscolo si può usare 'toupper' (man 3 toupper).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//Funzione che converte una stringa in maiuscolo
void strtoupper(const char* in, size_t len, char* out){
    for(size_t i = 0; i < len; i++){
        //converto ogni caratter in maiuscolo usando toupper
        out[i] = toupper((unsigned char)in[i]);
    }
    out[len] = '\0'; //terminatore di fine stringa 
}

int main(int argc, char* argv[]){
    if (argc < 2)
    {
        printf("Uso: %s <stringa1> <stringa2> ...\n",argv[0]);
        return 1; //uscita con errore
    }

    //itero su tutti gli argomenti passati al programma
    for (int i = 1; i < argc; i++)
    {
        size_t len = strlen(argv[i]); //otteniamo la lunghezza della stringa

        //allocazione dinamica della memoria per la stringa in maiuscolo (+1 per il terminatore)ù
        char *out = malloc (len + 1);
        if(!out){
            perror("Errore di allocazione di memoria");
            return 1;
        }

        //converto la stringa usando toupper
        strtoupper(argv[i], len, out);

        //stampo il risultato
        printf("Originale: %s -> Maiuscolo: %s\n",argv[i], out);

        //libero la memoria allocata
        free(out);
    }
    
    
}