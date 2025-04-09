#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
/** Elemento della coda
*
*/
typedef struct Node{
    void *data; //puntatore al dato memorizzato
    struct Node *next;    //puntatore al nodo successivo
}Node_t;

/** Struttura della coda
*
*/
typedef struct Queue{
    Node_t *head;   //elemento di testa (dummy node)
    Node_t *tail;   //elemento di coda
    unsigned long q_len;  //lunghezza
    pthread_mutex_t qlock; //mutex per la mutua esclusione sugli accessi
    pthread_cond_t qcond; //Variabile di condizione per la sincronizzazione
}Queue_t;

/** Inizializza una nuova coda thread-safe
* Alloca la struttura della coda e un nodo sentinella (dummy) che funge da punto
* iniziale. Inizializza inolte il mutex e la cond. var. necessari per la synchro
*
*  \retval NULL se si sono verificati probelmi nell'allocazione (errno settato)
*  \retval puntatore alla coda allocata
*/
Queue_t *initQueue();

/** Cancella una coda allocata con initQueue. Deve essere chiamata da
*   un solo thread
*
*  \param q puntatore alla coda da cancellare
*/
void deleteQueue(Queue_t *q);

/** Inserisce un dato nella coda.
 *
 * La funzione crea un nuovo nodo contenente il puntatore ai dati passati e lo
 * inserisce in fondo alla coda. Dopo l'inserimento, viene segnalata la condizione
 * per risvegliare eventuali thread in attesa di un nuovo elemento.
 *
 * \param q Puntatore alla coda.
 * \param data Puntatore ai dati da inserire.
 * \retval 0 in caso di successo.
 * \retval -1 in caso di errore (errno settato).
 */
int push(Queue_t *q, void *data);

/** Estrae un dato dalla coda.
 *
 * Se la coda è vuota, la funzione attende (bloccando il thread) fino a quando
 * un nuovo elemento viene inserito. L'operazione è protetta da mutex per garantire
 * la sicurezza nei confronti dei thread concorrenti.
 *
 * \param q Puntatore alla coda.
 * \retval Puntatore ai dati contenuti nel nodo rimosso, oppure NULL in caso di errore.
 */
void *pop(Queue_t *q);

/** Ritorna il dato in testa alla coda senza estrarlo.
 *
 * Se la coda è vuota, viene restituito NULL senza bloccare il thread chiamante.
 *
 * \param q Puntatore alla coda.
 * \retval Puntatore ai dati del primo elemento, oppure NULL se la coda è vuota o in caso di errore.
 */
void *top(Queue_t *q);

/** Ritorna la lunghezza attuale della coda.
 *
 * La lettura avviene in mutua esclusione per garantire la consistenza del valore letto.
 *
 * \param q Puntatore alla coda.
 * \retval Numero di elementi presenti nella coda.
 */
unsigned long length(Queue_t *q);

#endif //QUEUE_H
