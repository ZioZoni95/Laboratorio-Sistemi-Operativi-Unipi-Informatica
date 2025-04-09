#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <queue.h>

/**
* Definizioni delle capacità dei "porta componenti" e del numero massimo
*/
int R_CAPACITY = 5; //capacità massima del porta ruote
int T_CAPACITY = 3; // capacità massima del porta telai
int  B_MAX = 10; //Numero di biciclette da assemblare per terminare

/**
* Variabili globali
*/
Queue_t *qruote = NULL; //Porta-ruote (coda per le ruote)
Queue_t *qtelai = NULL; //Porta-telai (coda per i telai)

pthread_mutex_t mux_deposito = PTHREAD_MUTEX_INITIALIZER; //mutex per proteggere il contatore delle biciclette
int bici_prodotte = 0; //Condatore delle biciclette assemblate

// ------------------------------------------------------------------
// Funzioni "dummy" per simulare le operazioni di produzione e assemblaggio
// ------------------------------------------------------------------
/**
* Simula la produzione della ruota
* @param r L'ID della ruota da produrre
*
* NOTA: Si introduce una breve pausa per simulare il tempo di produzione
*/
void produci_ruota(long r){
  printf("Produzione ruota %ld\n", r);
  usleep(5000); //pausa di 50ms
}

/**
* Simula la produzione del telaio
* @param t L'ID del telaio da produrre
*
* NOTA: Si introduce una breve pausa per simulare il tempo di produzione
*/
void produci_telaio(long t){
  printf("Produzione telaio %ld\n", t);
  usleep(7000); //pausa di 70ms
}

/**
* Simula l'assemblaggio di una bici
*
* NOTA: si introduce una pausa per rappresentare il tempo di assemblaggio
*/
void assembla_bici(){
  printf("Assemblo bicicletta\n");
}

// ------------------------------------------------------------------
// Funzione per controllare se il lavoro deve terminare
// ------------------------------------------------------------------
/**
* Controlla se il numero di biciclette assemblate ha raggiunto il limite.
* \return true se la produzione è terminata, false altrimenti.
*/
bool termina_produzione(){
  bool isDone;
  if(pthread_mutex_lock(&mux_deposito) != 0){
    perror("pthread_mutex_lock");
    exit(EXIT_FAILURE);
  }
  isDone = (bici_prodotte >= B_MAX);
  if(pthread_mutex_unlock(&mux_deposito) != 0){
    perror("pthread_mutex_unlock");
    exit(EXIT_FAILURE);
  }
  return isDone;
}

// ------------------------------------------------------------------
// Funzioni eseguite dai thread
// ------------------------------------------------------------------
/**
 * Funzione eseguita dal thread Francesco.
 * Produce ruote e le deposita nel porta-ruote (coda qruote).
 */
void *Francesco(void *arg){
  long r = 0;
  while(!termina_produzione()){
    produci_ruota(r);

    long *pr = malloc(sizeof(long));
    if(pr == NULL){
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    *pr = r;

    printf("[Francesco] Provo a depositare ruota %ld...\n", r);
    if(push(qruote, pr) != 0){
      perror("[Francesco] Errore nella push");
      free(pr);
    } else {
      printf("[Francesco] Ruota %ld depositata con successo\n", r);
    }

    r++;
  }
  printf("[Francesco] Produzione ruote terminata.\n");
  return NULL;
}


/**
 * Funzione eseguita dal thread Federico.
 * Produce telai e li deposita nel porta-telai (coda qtelai).
 */
void *Federico(void *arg){
  long t = 0;
  while(!termina_produzione()){
    produci_telaio(t);

    long *pt = malloc(sizeof(long));
    if(pt == NULL){
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    *pt = t;

    printf("[Federico] Provo a depositare telaio %ld...\n", t);
    if(push(qtelai, pt) != 0){
      perror("[Federico] Errore nella push");
      free(pt);
    } else {
      printf("[Federico] Telaio %ld depositato con successo\n", t);
    }

    t++;
  }
  printf("[Federico] Produzione telai terminata.\n");
  return NULL;
}


/**
 * Funzione eseguita dal thread Franco.
 * Preleva due ruote dal porta-ruote e un telaio dal porta-telai per assemblare una bicicletta.
 */
void *Franco(void *arg){
  while(!termina_produzione()){
    printf("[Franco] In attesa di due ruote...\n");
    long *pr1 = (long *)pop(qruote);
    long *pr2 = (long *)pop(qruote);
    printf("[Franco] Ruote ottenute: %ld, %ld\n", *pr1, *pr2);

    printf("[Franco] In attesa di un telaio...\n");
    long *pt = (long *)pop(qtelai);
    printf("[Franco] Telaio ottenuto: %ld\n", *pt);

    if(pr1 == NULL || pr2 == NULL || pt == NULL){
      fprintf(stderr, "[Franco] Errore durante la pop()\n");
      break;
    }

    long ruota1 = *pr1;
    long ruota2 = *pr2;
    long telaio = *pt;

    free(pr1);
    free(pr2);
    free(pt);

    assembla_bici();

    if(pthread_mutex_lock(&mux_deposito) != 0){
      perror("pthread_mutex_lock");
      exit(EXIT_FAILURE);
    }
    bici_prodotte++;
    printf("[Franco] Bicicletta %d assemblata con ruote %ld, %ld e telaio %ld\n",
            bici_prodotte, ruota1, ruota2, telaio);
    if(pthread_mutex_unlock(&mux_deposito) != 0){
      perror("pthread_mutex_unlock");
      exit(EXIT_FAILURE);
    }
  }

  printf("[Franco] Assemblaggio terminato.\n");
  return NULL;
}


void print_usage() {
  printf("Benvenuto nella fabbrica di biciclette di ZioZoni95!\n");
  printf("Uso:\n");
  printf("  ./prod_bici                 (usa valori di default)\n");
  printf("  ./prod_bici 5 5 10          (R_CAPACITY, T_CAPACITY, B_MAX)\n");
  printf("  ./prod_bici R_CAPACITY=5 T_CAPACITY=5 B_MAX=10\n");
  printf("\nTutti i valori devono essere numeri interi positivi.\n\n");
}

// ------------------------------------------------------------------
// Funzione main: inizializza le risorse, crea i thread e attende il loro termine
// ------------------------------------------------------------------
int main(int argc, char *argv[]) {
  print_usage();

  if (argc == 4) {
    // Formato compatto: ./prod_bici 5 5 10
    R_CAPACITY = atoi(argv[1]);
    T_CAPACITY = atoi(argv[2]);
    B_MAX = atoi(argv[3]);

    if (R_CAPACITY <= 0 || T_CAPACITY <= 0 || B_MAX <= 0) {
      printf("Errore: tutti i valori devono essere numeri interi positivi.\n");
      print_usage();
      exit(EXIT_FAILURE);
    }

    printf("INFO: Avvio con parametri personalizzati (formato compatto): R_CAPACITY=%d, T_CAPACITY=%d, B_MAX=%d\n",
           R_CAPACITY, T_CAPACITY, B_MAX);

  } else if (argc > 1) {
    // Formato esteso: ./prod_bici R_CAPACITY=5 T_CAPACITY=5 B_MAX=10
    for (int i = 1; i < argc; i++) {
      if (strncmp(argv[i], "R_CAPACITY=", strlen("R_CAPACITY=")) == 0) {
        R_CAPACITY = atoi(argv[i] + strlen("R_CAPACITY="));
        if (R_CAPACITY <= 0) {
          printf("Errore: R_CAPACITY deve essere un numero positivo.\n");
          print_usage();
          exit(EXIT_FAILURE);
        }
      } else if (strncmp(argv[i], "T_CAPACITY=", strlen("T_CAPACITY=")) == 0) {
        T_CAPACITY = atoi(argv[i] + strlen("T_CAPACITY="));
        if (T_CAPACITY <= 0) {
          printf("Errore: T_CAPACITY deve essere un numero positivo.\n");
          print_usage();
          exit(EXIT_FAILURE);
        }
      } else if (strncmp(argv[i], "B_MAX=", strlen("B_MAX=")) == 0) {
        B_MAX = atoi(argv[i] + strlen("B_MAX="));
        if (B_MAX <= 0) {
          printf("Errore: B_MAX deve essere un numero positivo.\n");
          print_usage();
          exit(EXIT_FAILURE);
        }
      } else {
        printf("Errore: argomento non valido '%s'\n", argv[i]);
        print_usage();
        exit(EXIT_FAILURE);
      }
    }

    printf("INFO: Avvio con parametri personalizzati (formato esteso): R_CAPACITY=%d, T_CAPACITY=%d, B_MAX=%d\n",
           R_CAPACITY, T_CAPACITY, B_MAX);
  } else {
    // Nessun argomento: uso valori di default
    printf("WARNING: AVVIO con valori di default\n");
  }

  /* Inizializza le code per ruote e telai */
  qruote = initQueue();
  if (qruote == NULL) {
    perror("Errore in initQueue per qruote");
    exit(EXIT_FAILURE);
  }
  qtelai = initQueue();
  if (qtelai == NULL) {
    perror("Errore in initQueue per qtelai");
    exit(EXIT_FAILURE);
  }

  pthread_t thFrancesco, thFederico, thFranco;

  /* Crea i thread */
  if (pthread_create(&thFrancesco, NULL, Francesco, NULL) != 0) {
    perror("Errore nella creazione del thread Francesco");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&thFederico, NULL, Federico, NULL) != 0) {
    perror("Errore nella creazione del thread Federico");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&thFranco, NULL, Franco, NULL) != 0) {
    perror("Errore nella creazione del thread Franco");
    exit(EXIT_FAILURE);
  }

  /* Attende il termine dei thread */
  if (pthread_join(thFrancesco, NULL) != 0) {
    perror("Errore in pthread_join per Francesco");
    exit(EXIT_FAILURE);
  }
  if (pthread_join(thFederico, NULL) != 0) {
    perror("Errore in pthread_join per Federico");
    exit(EXIT_FAILURE);
  }
  if (pthread_join(thFranco, NULL) != 0) {
    perror("Errore in pthread_join per Franco");
    exit(EXIT_FAILURE);
  }

  printf("Produzione terminata. Totale biciclette assemblate: %d\n", bici_prodotte);

  /* Libera le risorse allocate per le code */
  deleteQueue(qruote);
  deleteQueue(qtelai);

  return 0;
}