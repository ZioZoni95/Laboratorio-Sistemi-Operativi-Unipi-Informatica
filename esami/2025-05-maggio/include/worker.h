#ifndef WORKER_H
#define WORKER_H

#include "common.h"

// Funzione eseguita da ciascun thread Worker (da 0 a P-1): partizionamento (solo Worker 0),
// sorting dei task prelevati dalla coda, barriera e merge a passi sincroni.
// `args` punta a una struttura ThreadArgs.
void *worker_thread(void *args);

#endif // WORKER_H
