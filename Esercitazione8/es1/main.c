#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <queue.h>

Queue queue;
int K, M, N;

//funzione eseguita dai thread produttori
void *produttore(void *arg){
    int id = *(int*)arg;
    int messages = K / M;
    for(int i = 0; i< messages; i++){
        int value = id * 100 + i; //Identificatore del messaggio
        enqueue(&queue, value);
        printf("Produttore %d ha inserito %d\n", id, value);
    }
    free(arg);
    return NULL;
}