#include "unbounded_fifo.h"

// Definizione della struttura del nodo della coda
struct node
{
	void* data;         // Puntatore ai dati memorizzati nel nodo
	struct node* next;  // Puntatore al nodo successivo nella coda
};

// Definizione della struttura della coda
struct queue
{
	size_t data_size;      // Dimensione dei dati memorizzati in ogni nodo
	struct node* first_node; // Puntatore al primo nodo (dummy node)
	struct node* last_node;  // Puntatore all'ultimo nodo effettivo
};

// Funzione per creare una nuova coda
struct queue* create_queue(size_t dim)
{
	struct node* head;
	// Allocazione di memoria per il nodo dummy (nodo fittizio che funge da testa della coda)
	EXIT_IF_NULL(head, (struct node*) malloc(sizeof(struct node)), "malloc");

	head->data = NULL; // Il nodo dummy non contiene dati
	head->next = NULL; // Non ha un nodo successivo inizialmente

	struct queue* q;
	// Allocazione di memoria per la struttura della coda
	EXIT_IF_NULL(q, (struct queue*) malloc(sizeof(struct queue)), "malloc");
	q->data_size = dim;    // Imposta la dimensione dei dati che la coda conterrà
	q->first_node = head;  // Il primo nodo punta al nodo dummy
	q->last_node = head;   // L'ultimo nodo è inizialmente il nodo dummy

	return q; // Restituisce il puntatore alla nuova coda
}

// Funzione per verificare se la coda è vuota
bool queue_is_empty(struct queue* q)
{
	return q->first_node == q->last_node; // La coda è vuota se first_node e last_node coincidono
}

// Funzione per inserire un nuovo elemento nella coda
void push(struct queue** q, void* data)
{
	struct node* new_node;
	// Allocazione di memoria per il nuovo nodo
	EXIT_IF_NULL(new_node, (struct node*) malloc(sizeof(struct node)), "malloc");
	// Allocazione di memoria per i dati del nodo
	EXIT_IF_NULL(new_node->data, (void*) malloc((*q)->data_size), "malloc");
	// Copia dei dati forniti nel nuovo nodo
	memcpy(new_node->data, data, (*q)->data_size);
	new_node->next = NULL; // Il nuovo nodo è l'ultimo, quindi non ha successivi

	// Se la coda è vuota (contiene solo il nodo dummy)
	if ((*q)->last_node == (*q)->first_node)
	{
		(*q)->first_node->next = new_node; // Il nodo dummy ora punta al nuovo nodo
		(*q)->last_node = new_node;        // L'ultimo nodo diventa il nuovo nodo
	}
	else
	{
		(*q)->last_node->next = new_node; // L'ultimo nodo attuale punta al nuovo nodo
		(*q)->last_node = new_node;       // Aggiorna il puntatore all'ultimo nodo
	}
}

// Funzione per estrarre un elemento dalla coda
void* pop(struct queue* q)
{
	// Se la coda è vuota, restituisce NULL
	if (q->first_node == q->last_node)
		return NULL;

	struct node* tmp = q->first_node->next; // Nodo che verrà rimosso (primo nodo dopo il dummy)

	// Se c'è più di un elemento nella coda
	if (tmp->next != NULL)
		q->first_node->next = tmp->next; // Il nodo dummy ora punta al nodo successivo
	else
	{
		// Se c'era un solo elemento, la coda torna vuota
		q->first_node->next = NULL;
		q->last_node = q->first_node;
	}

	void* data;
	// Allocazione di memoria per restituire i dati estratti
	EXIT_IF_NULL(data, (void*) malloc(q->data_size), "malloc");
	memcpy(data, tmp->data, q->data_size); // Copia i dati dal nodo estratto

	free(tmp->data); // Libera la memoria occupata dai dati del nodo estratto
	free(tmp);       // Libera la memoria del nodo estratto

	return data; // Restituisce i dati estratti
}

// Funzione per liberare la memoria della coda
void free_queue(struct queue* q)
{
	if (!q) // Se la coda è già NULL, termina la funzione
		return;

	// Libera tutti i nodi della coda
	while (q->first_node->next != NULL)
	{
		struct node* tmp = q->first_node->next; // Nodo da eliminare
		q->first_node->next = (q->first_node->next)->next; // Aggiorna il puntatore al successivo

		free(tmp->data); // Libera la memoria dei dati del nodo
		free(tmp);       // Libera la memoria del nodo
	}

	free(q->first_node); // Libera il nodo dummy
	free(q);             // Libera la struttura della coda
}
