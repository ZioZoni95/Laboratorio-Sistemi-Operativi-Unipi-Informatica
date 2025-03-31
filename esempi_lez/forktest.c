/*un piccolo esempio: forktest*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
/*
int main(void){
    int pid;
    printf("Inizio\n");
    pid = fork();
    printf("%d: Ho rievuto: %d\n", getpid(),pid);
    return 0;
}
    */
//Versione per duplicazione memoria dei processi

int main(void){
    int pid;
    printf("Inizio\n");

    fflush(stdout); //necessaria perchè printf duplica il buffer

    int *a = malloc(sizeof(int)); //puntatore allocato sull'heap settato a 1
    *a = 1;
    

    pid = fork();
    printf("%d: Ho ricevuto %d (a=%d , a = %p)\n", getpid(), pid, *a, a); //stampiamo anche i valori di a e del puntatore

    if(pid == 0) *a = 2;
    sleep(1);
    //cosa succede dopo la modifica di a?
    printf("%d:  Svegliato: %d (*a = %d, *a = %p)\n",getpid(), pid, *a, a);

    return 0;
}    

//Alla fine post esecuzione:
/*
./testfork 
Inizio
17663: Ho ricevuto 17664 (a=1 , a = 0x57a9881d76b0)
17664: Ho ricevuto 0 (a=1 , a = 0x57a9881d76b0)
17663:  Svegliato: 17664 (*a = 1, *a = 0x57a9881d76b0)
17664:  Svegliato: 0 (*a = 2, *a = 0x57a9881d76b0)

si nota che il figlio è cambiato ma il padre è rimasto come stava, stesso spazio di indirizzamento 
ma copia dello spazio di indirizzamento
*/