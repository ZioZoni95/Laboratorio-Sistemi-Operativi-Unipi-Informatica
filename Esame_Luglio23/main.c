//main.c - Creazione e gestione dei thread
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sync.h>

void *scrittore(void *arg){
    accediT1T2();
    printf("Scrittore %ld in sezione critica.\n",(long)arg);
    sleep(1);
    rilasciaT1T2();
    return NULL;
}

void *lettore(void *arg){
    accediT3T4();
    printf("Lettore %ld in sezione critica.\n", (long)arg);
    sleep(1);
    rilasciaT3T4();
    return NULL;
}

int main(){
    pthread_t threads[4];

    sem_init(&mux, 0, 1);
    sem_init(&waitT1T2, 0, 0);
    sem_init(&waitT3T4, 0, 0);

    pthread_create(&threads[0], NULL, scrittore, (void *)1);
    pthread_create(&threads[1], NULL, scrittore, (void *)2);
    pthread_create(&threads[2], NULL, lettore, (void *)3);
    pthread_create(&threads[3], NULL, lettore, (void *)4);

    for(int i = 0; i < 4; i++){
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mux);
    sem_destroy(&waitT1T2);
    sem_destroy(&waitT3T4);

    return 0;
}

//DA FIXARE L'OUTPUT FINALE