/*Esercitazione 3
Esercizio 1: libtokenizer.a
Inserire in un file 'tokenizer.h' la dichiarazione di due funzioni 'tokenizer' e 'tokenizer_r'. 
'tokenizer' ha la stessa interfaccia di 'tokenizer_r' ma mentre la prima usa al suo interno 'strtok', 
la seconda usa 'strtok_r'. Inserire nel file 'tokenizer_lib.c' l'implementazione delle due funzioni.
Creare quindi una libreria statica 'libtokenizer.a' che offre l'implementazione delle due funzioni.
Scrivere un programma main (es. tokenizer_main.c) che utilizza una delle due (o entrambe le) funzioni,
il programma includerà il file 'tokenizer.h' e verrà linkato alla libreria statica 'libtokenizer.a'.
*/

#include <stdio.h>
#include "../inc/tokenizer.h"

int main(int argc, char *argv[]){
    (void)argc; //evita warning per argc
    (void)argv; //Evita warning per argv

    char str[] = "Sto,testando,il,programma,main"; //stringa da tokenizzare
    char *token;

    printf("Using tokenizer:\n");
    token = tokenizer(str, ","); //Prima chiamata a tokenizer
    while(token != NULL){
        printf("%s\n",token);
        token = tokenizer(NULL, ","); //Chiamata successiva con NULL
    }

    char str2[] = "Sono,Asus,mi,trovo,qui"; //Seconda stringa da tokenizzare
    char *saveptr;

    printf("\nUsing tokenizer_r:\n");
    token = tokenizer_r(str2, ",", &saveptr); //Prima chiamata a tokenizer_r
    while(token != NULL){
        printf("%s\n", token);
        token = tokenizer_r(NULL, ",", &saveptr); //Chiamate successive
    }
    return 0;
}