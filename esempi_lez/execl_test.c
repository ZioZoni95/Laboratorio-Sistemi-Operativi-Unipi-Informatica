#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/*execl test*/
int main (void){
    printf("The quick brown fox jumped over");
    execl("/bin/echo", "echo", "the" , "lazy", "dogs", (char*)NULL);

    /*se execl ritorna si è verificato un errore*/
    perror("execl");
    return 1;
}