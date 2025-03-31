#include <pthread.h>

static int x; /*la variabile condivisa*/


static void *myfun(void *arg){
    while(1){
        printf("Secondo thread: x=%d\n",++x);
        sleep(1);
    }
}

int main(void){
    pthread_t tid;
    int err;
    if(err = pthread_create(&tid, NULL, &myfun, NULL)!= 0){
        /*gestione errore*/
    }
    else{
        while(1){
            printf("Primo thread: x=%d\n", ++x);
            sleep(1);
        }
    }
}