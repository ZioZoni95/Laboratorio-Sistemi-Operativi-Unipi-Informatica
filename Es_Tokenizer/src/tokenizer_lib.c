#include <stdio.h>
#include <string.h>
#include "../inc/tokenizer.h"

//Implementazione della funzione tokenizer usando strtok
char *tokenizer(char *str, const char *delim){
    return strtok(str, delim); // Usa strtok per tokenizzare la stringa
}

//Implementazione della funzione tokenizer_r usando strtok_r
char *tokenizer_r(char *str, const char *delim, char **saveptr){
    return strtok_r(str, delim, saveptr); // Usa strtok_r per supportare thread safety
}