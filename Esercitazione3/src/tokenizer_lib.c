#include <stdio.h>
#include <string.h>
#include "../inc/tokenizer.h"

//Implementazione della funzione tokenizer usando strtok
void tokenizer(char *str, const char *delim, char **token){
    *token = strtok(str, delim);
}

//Implementazione della funzione tokenizer_r usando strtok_r
void tokenizer_r(char *str, const char *delim, char **saveptr, char **token){
    *token = strtok_r(str,delim,saveptr);
}