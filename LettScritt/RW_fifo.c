//definizione della feature POSIX per utilizzare le API a partire da 2001
#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>


#include <queue.h> //Implementazione di una coda (FIFO) per la gestione degli accessi

//Variabili globali per il controllo dell'esecuzione
static int N; //Numero di iterazioni (o accessi) per i writer
static int stop; //Varidabile di terminazione (conta i writer attivi)
static unsigned long t0; //Tempo iniziale in microsecondi

/**
* @brief Restituisce il tempo corrente in microsecondi
*
* Utilizza gettimeofday() per ottenere il tempo corrente e lo converte in microsecondi
*
* @return Tempo corrente in microsecondi
*/
static inline unsigned long getusec(){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned long) (tv.tv_sec * 1e6 + tv.tv_usec);
}

//Mutex globale per proteggere le variabili condivise e la coda di ordinamento
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//Conta il numero di lettori attivi in sezione critica
static int activeReaders = 0;
/** Stato corrente della risorsa condivisa:
*     -1: nessuno in sezione critica
*      0: lettura in corso
*      1: scrittura in corso
*/
static int state = -1;

/**
* Struttura per gestire l'ordine FIFO tra le richieste di accesso.
* Ogni elemento della coda contiene:
*    - Una variabile di condizione per attendere il proprio "turno"
*    - Un flag che indica il tipo di accesso richiesto:
+        -- 0: lettore
+        -- 1 scrittore
*/
typedef struct _ordering {
  pthread_cond_t ord; //variabile di condizione associata all'accaesso
  int rw; //Tipo di accesso
}ordering_t;

//Coda FIFO per gestire l'ordine di accesso (richieste in sospeso)
Queue_t *orderingQ;

/**
* @brief Funzione per iniziare l'accesso in lettura (startRead)
*
* Ogni lettore deve:
*    -> Acquisire il mutex globale per accedere in modo sicuro alle variabili condivise
*    -> Verificare se c'è già uno scrittore in corso (state > 0) oppure se c'è qualcun
*       altro in attesa (orderingQ non vuoto). In questo caso si inserisce nella coda
*       di ordinamento e si attende finchè non sarà il proprio turno
*    -> Una volta entrato nella sezione critica,aumenta il contatore dei lettori attivi e
*       imposta lo stato a 0 (lettura)
*
* @param id Identificativo del lettore (debug/log)
*/
void startRead(int id){
  pthread_mutex_lock(&mutex);
  //Se c'è un writer attivo (state > 0) oppure qualcuno in coda, il lettore si mette in wait
  if(top(orderingQ) != NULL || state > 0){
    //crea un nuovo elemento per la coda di ordinamento specificando che è una richiesta di lettura
    ordering_t *o = malloc(sizeof(ordering_t));
    assert(o);

    pthread_cond_init(&o->ord, NULL);
    o->rw = 0; //sono lettore
    if(push(orderingQ, o) < 0) abort();

    //Attende fino a quando non viene segnalato e finchè non ci sono scrittori attivi
    do{
      pthread_cond_wait(&o->ord, &mutex);
    }while (state > 0); //se ci sono scrittori, continua ad attendere

    //Verifica che lo stato sia compatibile con un accesso in lettura
    assert(state == -1 || state == 0);
    //Rimuove il proprio elemento dalla coda FIFO
    ordering_t *o_tmp = pop(orderingQ);
    assert(o == o_tmp);
    pthread_cond_destroy(&o->ord);
    free(o);

    //Se c'è un altro lettore in wait lo sveglia
    o = top(orderingQ);
    if(o && o->rw == 0){
      pthread_cond_signal(&o->ord);
    }
  }
  //Incrementa il numero di lettori attivi e imposta lo stato a 0 (lettura)
  activeReaders++;
  state = 0;
  pthread_mutex_unlock(&mutex);
}

/**
* @brief Funzione per terminare l'accesso in lettura (doneRead).
*
* Ogni lettore che termina:
*  - Acquisisce il mutex per accedere alle variabili condivise.
*  - Decrementa il contatore dei lettori attivi.
*  - Se è l'ultimo lettore (activeReaders==0), resetta lo stato a -1 e,
*    se in coda c’è uno scrittore, lo segnala.
*
* @param id Identificativo del lettore.
*/
void doneRead(int id){
  pthread_mutex_lock(&mutex);
  activeReaders--;
  assert(activeReaders >= 0);
  //Se sono l'ultimo lettore in uscita
  if(activeReaders == 0){
    state = -1; //reset dello stato
    //se c'è un elemento in coda (che deve essere uno scrittore)
    if(length(orderingQ) > 0){
      ordering_t *o = top(orderingQ);
      assert(o && o->rw == 1);
      //Sveglia lo scrittore in attesa
      pthread_cond_signal(&o->ord);
    }
  }
  pthread_mutex_unlock(&mutex);
}

/**
* @brief Funzione per iniziare l'accesso in scrittura (startWrite).
*
* Ogni scrittore:
*  - Acquisisce il mutex globale.
*  - Se ci sono altri lettori attivi o uno scrittore in corso (state >= 0),
*    inserisce la propria richiesta nella coda e attende.
*  - Una volta svegliato e verificato che lo stato è -1 (nessuno in sezione critica),
*    rimuove la propria richiesta dalla coda e imposta lo stato a 1 (scrittura).
*
* @param id Identificativo dello scrittore.
*/
void startWrite(int id){
  //Se c'è già uno o più lettori oopure uno scrittore attivo
  if(state >= 0){
    //Crea un nuovo elemento per la coda, indicando che è una richiesta di scrittura
    ordering_t *o = malloc(sizeof(ordering_t));
    pthread_cond_init(&o->ord, NULL);
    o->rw = 1; //indica scrittura
    if(push(orderingQ, o) < 0) abort();

    //attende finchè lo stato non diventa -1 (nessuno in ssezione critica)
    do{
      pthread_cond_wait(&o->ord, &mutex);
      //Viene svegliato con state = -1 per avere la precedenza su eventuali lettori arrivati nel frattempo
    }while (state >= 0);
    assert(state == -1);
    //Rimuove la propria richiesta dalla coa
    ordering_t *o_tmp = pop(orderingQ);
    assert(o == o_tmp);
    pthread_cond_destroy(&o->ord);
    free(o);
  }
  //Imposta lo stato a 1 per indicare che un wirter sta entrando nella sezione critica
  state = 1;
  pthread_mutex_unlock(&mutex);
}

/**
* @brief Funzione per terminare l'accesso in scrittura (doneWriter).
*
* Lo scrittore che termina:
*  - Acquisisce il mutex.
*  - Imposta lo stato a -1 per bloccare eventuali lettori che potrebbero entrare
*    immediatamente dopo la terminazione (per garantire l'ordine).
*  - Se esiste una richiesta in coda, la sveglia.
*
* @param id Identificativo dello scrittore.
*/

void doneWriter(int id){
  pthread_mutex_lock(&mutex);
  assert(state == 1);

  //Reset dello stato a -1 per evitarer che lettori possano entrare
  // prima di dare la precedenza ai thread in coda
  state = -1;

  //Se c'è una richeista in coda (sia essa lettore o scrittore), la sveglia
  ordering_t *o = top(orderingQ);
  if(o != NULL) pthread_cond_signal(&o->ord);
  pthread_mutex_unlock(&mutex);
}

/**
  @brief Simula un lavoro che dura un certo tempo (in microsecondi).
*
* Utilizza nanosleep() per effettuare una pausa (sleep) del tempo specificato.
*
* @param us Durata del lavoro in microsecondi.
*/

void work (long us){
  struct timespec t = {0, us * 1000};
  nanosleep(&t, NULL);
}

/**
* @brief Funzione eseguita dai thread lettori.
*
* Ogni lettore:
*  - Cicla finché la variabile globale stop è positiva.
*  - Inizia la lettura chiamando startRead().
*  - Stampa un messaggio di entrata nella sezione critica, simula un lavoro,
*    stampa un messaggio di uscita e chiama doneRead().
*
* @param arg Identificativo del lettore (convertito in long).
* @return NULL.
*/

void *Reader(void *arg){
  long id = (long)arg;
  while(stop > 0){
    startRead(id);

    printf("READER%ld ENTRATO entrato t=%.2f\n", id, (getusec() - t0) / 1000.0);
    work(2000);
    printf("READER%ld USCITO dalla sezione critica\n", id);

    doneRead(id);
  }
  printf("READER%ld TERMINATO\n",id);
  return NULL;
}

void *Writer(void *arg){
  long id = (long)arg;
  for(int i = 0; i< N; ++i){
    startWrite(id);

    printf("WRITER%ld ENTRATO in sezione critica t=%.2f\n", id, (getusec() - t0) / 1000.0);
    work(6000);
    printf("WRITER%ld USCITO dalla sezione critica\n", id);

    //Se è l'ultima iterazione, decrementa stop per far terminare i lettori
    if(i + 1 == N)
      --stop;
    
    doneWriter(id);  
  }
  printf("WRITER TERMINATO\n");
  return NULL;
}

/**
* @brief Funzione main.
*
* Il main:
*  - Legge i parametri (numero di lettori, scrittori e iterazioni) dalla linea di comando.
*  - Inizializza le variabili globali e la coda di ordinamento.
*  - Crea i thread lettori e scrittori.
*  - Attende la terminazione di tutti i thread, libera le risorse allocate e termina.
*
* @param argc Numero di argomenti.
* @param argv Array degli argomenti.
* @return 0 in caso di successo, -1 in caso di errore.
*/
int main(int argc, char *argv[]){
  int R = 5;
  int W = 2;
  N = 100;
  if(argc > 1){
    if(argc != 4){
      fprintf(stderr, "Uso: %s [#R #W N]\n",argv[0]);
      return -1;
    }
    R = atoi(argv[1]);
    W = atoi(argv[2]);
    N = atoi(argv[3]);
  }
  //Limite ragionevole per il numero di thread
  if(R > 100) R = 100;
  if(W > 100) W = 100;

  stop = W;
  orderingQ = initQueue();
  t0 = getusec();

  //Alloca array per i thread lettori e scrittori
  pthread_t *readers = malloc(R * sizeof(pthread_t));
  pthread_t *writers = malloc(W * sizeof(pthread_t));
  if(!readers || !writers){
    fprintf(stderr, "memoria insufficiente\n");
    return -1;
  }

  //Creazione dei thread scrittori
  for(long i = 0; i < W; ++i){
    if(pthread_create(&writers[i], NULL, Writer, (void *)i) != 0){
      fprintf(stderr, "pthread_create Writer fallita\n");
      return -1;
    }
  }

  //Creazione dei thread lettori
  for(long i = 0; i < R; i++){
    if(pthread_create(&readers[i], NULL, Reader, (void *)i) != 0){
      fprintf(stderr, "pthread_create Reader fallita\n");
      return -1;
    }
  }
  
  //Attende la terminazione dei thread lettori
  for (long i = 0; i < R; ++i){
    if (pthread_join(readers[i], NULL) == -1){
      fprintf(stderr, "pthread_join failed\n");
    }
  }
  // Attende la terminazione dei thread scrittori
  for (long i = 0; i < W; ++i){
    if (pthread_join(writers[i], NULL) == -1){
      fprintf(stderr, "pthread_join failed\n");
    }
  }

  free(writers);
  free(readers);
  deleteQueue(orderingQ);
  return 0;
}