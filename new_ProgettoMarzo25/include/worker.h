#ifndef WORKER_H
#define WORKER_H

#include "threads_types.h"

/* Funzione eseguita dal thread Worker */
void *worker_thread(void *arg);

#endif
