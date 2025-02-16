/*
Esercizio 3: Pipeline concorrente con thread in C

Obiettivo:
Scrivere un programma che implementa una pipeline di tre thread:
1. Il primo thread legge una riga da un file e la invia al secondo thread.
2. Il secondo thread tokenizza la riga ricevuta e invia i token al terzo thread.
3. Il terzo thread stampa i token ricevuti sullo standard output.

La comunicazione tra i thread avviene tramite una coda FIFO unbounded.
*/

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "unbounded_fifo.h" // Include la libreria per la coda FIFO senza limiti
#include "wrappers.h" // Include funzioni wrapper per la gestione degli errori

#define N 2 // Numero di condizioni per la sincronizzazione
#define MAXTOKEN 128 // Dimensione massima di un token
#define MAXBUFFER 1024 // Dimensione massima del buffer per le righe lette dal file

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex per la sincronizzazione dei thread
pthread_cond_t** condition; // Array di variabili di condizione per la sincronizzazione

// Dichiarazione delle routine dei thread
static void* reading_routine(void*);
static void* tokenizing_routine(void*);
static void* printing_routine(void*);

// Code FIFO condivise tra i thread
struct queue** buffer_queue;
struct queue** token_queue;

// Variabili di stato per il controllo del flusso
bool reading_is_done = false;
bool tokenizing_is_done = false;
bool buffer_empty = true;
bool token_empty = true;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input filename>\n", argv[0]);
        return 1;
    }

    int err;
    pthread_t reading, tokenizing, printing;
    pthread_cond_t* _stacked_condition;
    EXIT_IF_NULL(_stacked_condition, (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * N), "malloc");
    condition = &(_stacked_condition);
    
    struct queue* _stacked_buffer_queue = create_queue(sizeof(char) * MAXBUFFER);
    buffer_queue = &_stacked_buffer_queue;
    struct queue* _stacked_token_queue = create_queue(sizeof(char) * MAXTOKEN);
    token_queue = &_stacked_token_queue;

    // Inizializza le variabili di condizione
    for (size_t i = 0; i < N; i++) {
        EXIT_IF_NOT_EXPECTED_VALUE(err, 0, pthread_cond_init(&((*condition)[i]), NULL), "pthread_cond_init");
    }
    
    FILE* fd;
    FOPEN(fd, argv[1], "r");

    // Creazione dei thread
    Pthread_create(&reading, NULL, &reading_routine, (void*) fd);
    Pthread_create(&tokenizing, NULL, &tokenizing_routine, NULL);
    Pthread_create(&printing, NULL, &printing_routine, NULL);

    // Attesa della terminazione dei thread
    EXIT_IF_NOT_EXPECTED_VALUE(err, 0, pthread_join(reading, NULL), "pthread_join");
    EXIT_IF_NOT_EXPECTED_VALUE(err, 0, pthread_join(tokenizing, NULL), "pthread_join");
    EXIT_IF_NOT_EXPECTED_VALUE(err, 0, pthread_join(printing, NULL), "pthread_join");

    fclose(fd);
    free(*condition);
    free_queue(*buffer_queue);
    free_queue(*token_queue);
    return 0;
}

// Routine per il thread di lettura
static void* reading_routine(void* arg) {
    FILE* fd = (FILE*) arg;
    char* buffer;
    EXIT_IF_NULL(buffer, (char*) calloc(MAXBUFFER, sizeof(char)), "calloc");
    char* dummy_buffer = buffer;

    while (fgets(buffer, MAXBUFFER, fd)) {
        Pthread_mutex_lock(&mutex);
        push(buffer_queue, (void*)buffer);
        if (buffer_empty) {
            buffer_empty = false;
            pthread_cond_signal(&((*condition)[0])); // Notifica il thread di tokenizzazione
        }
        Pthread_mutex_unlock(&mutex);
    }

    free(dummy_buffer);
    reading_is_done = true;
    return NULL;
}

// Routine per il thread di tokenizzazione
static void* tokenizing_routine(void* arg) {
    char* buffer;
    char* tmp_buffer;
    char* saveptr;
    char* token;

    while (true) {
        if (buffer_empty && reading_is_done) break;
        Pthread_mutex_lock(&mutex);
        if (buffer_empty) {
            pthread_cond_wait(&(*(condition)[0]), &mutex); // Aspetta dati dalla lettura
        }
        buffer = (char*) pop(*buffer_queue);
        if (queue_is_empty(*buffer_queue)) buffer_empty = true;
        Pthread_mutex_unlock(&mutex);

        tmp_buffer = buffer;
        token = strtok_r(buffer, " ", &saveptr);
        while (token != NULL) {
            Pthread_mutex_lock(&mutex);
            token[strcspn(token, "\n")] = 0; // Rimuove il newline finale
            if (token) {
                push(token_queue, (void*) token);
                if (token_empty) {
                    token_empty = false;
                    pthread_cond_signal(&((*condition)[1])); // Notifica il thread di stampa
                }
            }
            Pthread_mutex_unlock(&mutex);
            token = strtok_r(NULL, " ", &saveptr);
        }

        free(tmp_buffer);
    }
    tokenizing_is_done = true;
    return NULL;
}

// Routine per il thread di stampa
static void* printing_routine(void* arg) {
    while (true) {
        if (token_empty && tokenizing_is_done) break;
        Pthread_mutex_lock(&mutex);
        if (token_empty) {
            pthread_cond_wait(&((*condition)[1]), &mutex); // Aspetta dati dalla tokenizzazione
        }

        char* str = (char*) pop(*token_queue);
        if (queue_is_empty(*token_queue)) token_empty = true;
        fprintf(stdout, "POPPING AND PRINTING: %s\n", str);
        fflush(stdout);
        Pthread_mutex_unlock(&mutex);
        free(str);
    }
    return NULL;
}
