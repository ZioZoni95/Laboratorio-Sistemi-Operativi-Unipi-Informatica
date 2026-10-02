#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){
    //controllo gli argomenti
    if(argc != 3){
        fprintf(stderr, "Uso: %s stringa1 stringa2\n",argv[0]);
        return -1;
    }

    char *saveptr1, *saveptr2;
    char *token1 = strtok_r(argv[1]," ", &saveptr1);

    //Itera sui token della prima stringa
    while(token1){
        printf("%s\n", token1);

        char *token2 = strtok_r(argv[2], " ", &saveptr2);

        //itera sui token della seconda stringa
        while(token2){
            printf("%s\n", token2);
            token2 = strtok_r(NULL, " ", &saveptr2);
        }
        token1 = strtok_r(NULL, " ", &saveptr1);
    }
    return 0;
}