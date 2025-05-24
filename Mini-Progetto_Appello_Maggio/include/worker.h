#ifndef WORKER_H
#define WORKER_H

#include "common.h" // Include ThreadArgs, Task, etc.

// --- Dichiarazione Funzione Worker ---
// Funzione eseguita da ciascun thread Worker (da 0 a P-1).
// Implementa le fasi di sorting e merge parallelo come descritto
// nel testo d'esame.
// Riceve un puntatore a una struttura ThreadArgs come argomento.
void *worker_thread(void *args);

#endif // WORKER_H