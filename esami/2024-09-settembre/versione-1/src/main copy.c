#include <stdio.h>
#include <pthread.h>
#include "worker.h"
#include "queue.h"

int main(){
    queue_init(&qruote, R);
    queue_init(&qtelai, T);

    pthread_t thread1, thread2, thread3;
    pthread_create(&thread1, NULL, francesco, NULL);
    pthread_create(&thread2, NULL, federico, NULL);
    pthread_create(&thread3, NULL, franco, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);



    printf("Produzione terminata con %ld biciclette.\n", biciprodotte);
    return 0;
}