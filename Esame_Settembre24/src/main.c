#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "worker.h"
#include "queue.h"

void print_usage() {
    printf("Benvenuto nella fabbrica di biciclette di ZioZoni95!\n");
    printf("Uso: ./fabbrica_biciclette [R=<numero_ruote_prodotte>] [T=<numero_telai_prodotti>] [B=<numero_biciclette_assemblate>]\n");
    printf("DISCLAIMER: Usare lo script per ulteriori test oppure inserire manualmente i valori desiderati\n");
    //exit(1);
}

int main(int argc, char *argv[]){
    print_usage();
    // Se l'utente non ha passato argomenti, mostra i valori di default
    // Se l'utente non ha passato argomenti, mostra i valori di default
    if (argc == 1) {
        printf("WARNING: Avvio fabbricazione con valori di default R=5 T=3 B=10\n");
    }

    // Parsing degli argomenti da riga di comando
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "R=", 2) == 0) {
            R = atoi(argv[i] + 2);  // Imposta la capacità del porta ruote
            if (R <= 0) {
                printf("Errore: R deve essere un numero positivo.\n");
                print_usage();
            }
        } else if (strncmp(argv[i], "T=", 2) == 0) {
            T = atoi(argv[i] + 2);  // Imposta la capacità del porta telai
            if (T <= 0) {
                printf("Errore: T deve essere un numero positivo.\n");
                print_usage();
            }
        } else if (strncmp(argv[i], "B=", 2) == 0) {
            B = atoi(argv[i] + 2);  // Imposta il numero di biciclette
            if (B <= 0) {
                printf("Errore: B deve essere un numero positivo.\n");
                print_usage();
            }
        } else {
            printf("Errore: argomento non valido '%s'.\n", argv[i]);
            print_usage();
        }
    }
    queue_init(&qruote, R);
    queue_init(&qtelai, T);

    pthread_t thread1, thread2, thread3;
    int id1 = 1, id2 = 2 , id3 = 3;
    pthread_create(&thread1, NULL, wheelsproducer, &id1);
    pthread_create(&thread2, NULL, chassisproducer, &id2);
    pthread_create(&thread3, NULL, assemblerconsumer, &id3);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);



    printf("Produzione terminata con %ld biciclette.\n", biciprodotte);
    return 0;
}