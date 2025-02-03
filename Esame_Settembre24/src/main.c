#include <stdio.h>
#include <pthread.h>
#include "worker.h"
#include "queue.h"

int main(int argc, char *argv[]){
    queue_init(&qruote, R);
    queue_init(&qtelai, T);

    pthread_t thread1, thread2, thread3;
    int id1 = 1, id2 = 2 , id3 = 3;
    pthread_create(&thread1, NULL, francesco, &id1);
    pthread_create(&thread2, NULL, federico, &id2);
    pthread_create(&thread3, NULL, franco, &id3);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);



    printf("Produzione terminata con %ld biciclette.\n", biciprodotte);
    return 0;
}