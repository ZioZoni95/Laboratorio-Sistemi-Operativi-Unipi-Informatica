#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "worker.h"

// Variabili configurabili (default)
int R = 5;  // Capacità del porta ruote
int T = 3;  // Capacità del porta telai
int B = 10; // Numero di biciclette da produrre

pthread_mutex_t muxruote = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t muxtelaio = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t muxmagazzino = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cvruote = PTHREAD_COND_INITIALIZER;
pthread_cond_t cvtelai = PTHREAD_COND_INITIALIZER;
Queue_t qruote, qtelai;
long biciprodotte = 0;

bool termina(){
    pthread_mutex_lock(&muxmagazzino);
    bool r = (biciprodotte >= B);
    pthread_mutex_unlock(&muxmagazzino);
    return r;
}

void deposita_ruota(long r){
    pthread_mutex_lock(&muxruote);
    while(qruote.size >= R && !termina())
        pthread_cond_wait(&cvruote, &muxruote);
    queue_put(&qruote, r);
    pthread_cond_signal(&cvruote);    
    pthread_mutex_unlock(&muxruote);
    
}

void deposita_telaio(long t){
    pthread_mutex_lock(&muxtelaio);
    while(qtelai.size >= T && !termina())
        pthread_cond_wait(&cvtelai, &muxtelaio);
    queue_put(&qtelai, t);
    pthread_cond_signal(&cvtelai);
    pthread_mutex_unlock(&muxtelaio);
}

void preleva(long *ruota1, long *ruota2, long *telaio){
    pthread_mutex_lock(&muxruote);
    while(qruote.size < 2)
        pthread_cond_wait(&cvruote, &muxruote);
    *ruota1 = queue_get(&qruote);
    *ruota2 = queue_get(&qruote);
    pthread_cond_signal(&cvruote);
    pthread_mutex_unlock(&muxruote);

    pthread_mutex_lock(&muxtelaio);
    while(qtelai.size == 0)
        pthread_cond_wait(&cvtelai, &muxtelaio);
    *telaio = queue_get(&qtelai);
    pthread_cond_signal(&cvtelai);
    pthread_mutex_unlock(&muxtelaio);
}

void deposita_magazzino(){
    pthread_mutex_lock(&muxmagazzino);
    biciprodotte++;
    if(biciprodotte >= B){
        pthread_cond_signal(&cvruote);
        pthread_cond_signal(&cvtelai);
    }
    pthread_mutex_unlock(&muxmagazzino);
}

void *wheelsproducer(void *arg){
    int id = *(int *)arg;
    for(long r = 0;; r++){
        if(termina()) break;
        deposita_ruota(r);
        printf("Thread %d ha prodotto una ruota\n", id);
    }
    return NULL;
}

void *chassisproducer(void *arg){
    int id = *(int *)arg;
    for(long t = 0;; t++){
        if(termina()) break;
        deposita_telaio(t);
        printf("Thread %d ha prodotto un telaio\n", id);
    }
    return NULL;
}

void *assemblerconsumer(void *arg){
    int id = *(int *)arg;
    for(;;){
        if(termina()) break;
        long ruota1, ruota2, telaio;
        preleva(&ruota1, &ruota2, &telaio);
        deposita_magazzino();
        printf("Thread %d ha assemblato una bicicletta\n", id);
    }
    return NULL;
}


