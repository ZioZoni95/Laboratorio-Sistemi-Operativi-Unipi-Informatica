#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include <queue.h>

/**
*  @file queue.c
*  @brief Implementazione dell'interfaccia per una coda thread_safe
*
*  Questo file implementa una coda in cui produttori e consumatori possono accedere
*  in maniera concorrente grazie all'uso di mutex e variabili di condizione
*/

/* ------------------- Funzioni di utilità -------------------- */

/**
*  Alloca un nuovo nodo della coda
*
* @return Puntatore al nuovo nodo allocato, oppure NULL in caso di errore.
*/
static inline Node_t *allocNode(){
  return malloc(sizeof(Node_t));
}

/**
* Alloca una nuova struttura coda
*
* @return Puntatore alla nuova coda allocata,oppure NULL in caso di errore.
*/
static inline Queue_t *allocQueue(){
  return malloc(sizeof(Queue_t));
}

/**
* Libera la memoria associata ad un nodo
*  @param node Puntatore al nodo da liberare
*/
static inline void freeNode(Node_t *node){
  free((void *)node);
}

/**
* Acquisisce il mutex associato alla coda
*
* @param q Puntatore alla coda
*/
static inline void LockQueue(Queue_t *q){
  pthread_mutex_lock(&q->qlock);
}

/**
* Rilascia il mutex associato alla coda
*
* @param q Puntatore alla coda
*/
static inline void UnlockQueue(Queue_t *q){
  pthread_mutex_unlock(&q->qlock);
}

/**
* Attende sulla variabile di condizione della coda
*
* Questa funzione rilascia il mutex q->qlock e mette in attesa il thread
* fino a che non viene segnalato, quindi il mutex verrà nuovamente acquisito
* al risveglio
*
* @param q Puntatore alla coda
*/
static inline void UnlockQueueAndWait(Queue_t *q){
  pthread_cond_wait(&q->qcond, &q->qlock);
}

/**
* Segnala la variabile di condizione e rilascia il mutex associato.
*
* Dopo aver segnalato che potrebbe essere disponbibile un nuovo elemento,
* il mutex viene rilasciato
*
* @param q Puntatore alla coda
*/
static inline void UnlockQueueAndSignal(Queue_t *q){
  pthread_cond_signal(&q->qcond);
  pthread_mutex_unlock(&q->qlock);
}

/* ------------------- Interfaccia della coda ------------------ */

/**
* Inizializza una nuova coda thread-safe
*
* Alloca la struttura della coda e un nodo sentinella (dummy) che funge da
* punto iniziale. Inizializza inoltre il mutex e la variabile di condizione
* necessari per la sincronizzazione
*
* @return Puntatore alla coda inizializzata || NULL in caso di errore
*/
Queue_t *initQueue(){
  Queue_t *q = allocQueue();
  if(!q) return NULL;

  //Il nodo dummy semplifica l'inserimento e la rimozione
  q->head = allocNode();
  if(!q->head) return NULL;
  q->head->data = NULL;
  q->head->next = NULL;
  q->tail = q->head;
  q->q_len = 0;

  //inizializzazione del mutex
  if(pthread_mutex_init(&q->qlock, NULL) != 0){
    perror("pthread_mutex_init");
    return NULL;
  }
  //Inizializzazione della variabile di condizione
  if(pthread_cond_init(&q->qcond, NULL) != 0){
    perror("pthread_cond_init");
    pthread_mutex_destroy(&q->qlock);
    return NULL;
  }
  return q;
}

/**
* Libera tutte le risorse associate alla coda
*
* Rimuove tutti i nodi (dummy incluso) e distrugge il mutex e la
* variabile di condizione
*
* @param q Puntatore alla coda da eliminare
*/
void deleteQueue(Queue_t *q){
  //Libera tutti i nodi, tranne l'ultimo dummy
  while (q->head != q->tail) {
    Node_t *p = q->head;
    q->head = q->head->next;
    freeNode(p);
  }
  //Libera il nodo dummy residuo
  if(q->head) freeNode(q->head);

  //Distrugge il mutex e la cond. var.
  pthread_mutex_destroy(&q->qlock);
  pthread_cond_destroy(&q->qcond);
  free(q);
}

/**
* Inserisce un nuovo elemento in coda
*
* La funzione crea un nuovo nodo contenente il puntatore ai dati passati e lo
* inserisce in fondo alla coda. Dopo l'inserimento, viene segnalata la condizione
* per risvegliare eventuali thread in atttesa di un nuovo elemento.
*
* @param q Puntatore alla coda.
* @param data Puntatore ai dati da inserire
* @return 0 in caso di successo, -1 e setta errno in caso di errore
*/
int push(Queue_t *q, void *data){
  if((q == NULL) || (data == NULL)){
    errno = EINVAL;
    return -1;
  }
  Node_t *newNode = allocNode();
  if(!newNode){ return -1; }
  newNode->data = data;
  newNode->next = NULL;

  //Inserimento in coda in sezione critica
  LockQueue(q);
  q->tail->next = newNode;
  q->tail = newNode;
  q->q_len += 1;
  //Segnala la presenza di un nuovo elemento ed esce dalla sezione criitca
  UnlockQueueAndSignal(q);
  return 0;
}

/**
* Rimuove e restituisce il primo elemento disponibile in coda
*
* Se la coda è vuota, la funzione attende (bloccandosi) fino a quando un nuovo
* elemento non viene inserito. L'operazione è protetta da mutex per garantire la
* sicurezza nei confronti dei thread concorrenti.
*
* @param q Puntatore alla coda
* @return Puntatore ai dati contenuti nel nodo rimosso, oppure NULL in caso di errore
*/
void *pop(Queue_t *q){
  if(q == NULL){
    errno = EINVAL;
    return NULL;
  }
  LockQueue(q);
  //Attende finchè la coda è vuota (ossia solo il nodo dummy è presente)
  while(q->head == q->tail){
    UnlockQueueAndWait(q);
  }
  //A questo punto, la coda contiene almento un elemento
  assert(q->head->next);
  Node_t *newNode = q->head;
  void *data = q->head->next->data;
  //Avanza il puntatore head e aggiorna la lunghezza della coda
  q->head = q->head->next;
  q->q_len -= 1;
  assert(q->q_len >= 0);
  UnlockQueue(q);
  freeNode(newNode);
  return data;
}

/**
* Restituisce (senza rimuovere il primo elemento disponibile in coda
*
* Se la coda è vuota, viene restituito NULL senza bloccare il thread chiamante
*
* @param q Puntatore alla coda
* @return Puntatore ai dati del primo elemento, oppure NULL se la coda è vuota o in caso di errore
*/
void *top(Queue_t *q){
  if(q == NULL){
    errno = EINVAL;
    return NULL;
  }
  LockQueue(q);
  //Se la coda è vuota, esce subito rilasciando il mutex
  if(q->head == q->tail){
    UnlockQueue(q);
    return NULL;
  }
  //A questo punto, la coda contiene almento un elemento
  assert(q->head->next);
  void *data = (q->head->next)->data;
  UnlockQueue(q);
  return data;
}

/**
* Restituisce la lunghezza corrente della coda
*
* La lunghezza è il numero di elementi (nodi) presenti nella coda. La lettura
* avviene in mutua esclusione per garantire la consistenza del valore letto
*
* NOTA: In un ambiente concorrente, la lunghezza letta può risultare obsoleta
* subito dopo la lettura, per cui non è un valore affidabile per operazioni
* successive senza opportuna sincronizzazione
*
* @param q Puntatore alla coda
* @return Numero di elementi presenti nella coda
*/
unsigned long length(Queue_t *q){
  LockQueue(q);
  unsigned long len = q->q_len;
  UnlockQueue(q);
  return len;
}

