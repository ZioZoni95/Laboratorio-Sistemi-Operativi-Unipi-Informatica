# Pseudocodice C — tipi di dato e funzione del Worker

Prima parte della consegna richiesta dalla [traccia](Appello-Maggio.pdf): definizione dei tipi e funzione
eseguita dai Worker, in pseudocodice C. Si suppone già disponibile la coda `Q` (con `push`/`pop`) e la
funzione `barrier`. L'implementazione completa e compilabile è in [`src/worker.c`](src/worker.c).

## Tipi di dato

```c
// Un task è una partizione dell'array: coppia di indici (start, end), estremi inclusi.
typedef struct { long start; long end; } Task;

// Coda concorrente Q: push inserisce in coda; pop attende se è vuota e restituisce
// 0 solo quando la coda è vuota E chiusa (nessun altro task arriverà).
void push(Queue *Q, Task t);
int  pop (Queue *Q, Task *t);
void close(Queue *Q);          // il Worker 0 la invoca dopo l'ultimo push

// Argomenti di ciascun Worker.
typedef struct {
    int    id;                 // 0 .. P-1
    int    P;                  // numero di Worker
    long   Q_parts;            // numero di partizioni (la traccia assume Q_parts = P)
    long   N;                  // lunghezza dell'array
    int   *A, *B;              // array da ordinare e buffer di appoggio, entrambi di N elementi
    Queue *Q;                  // coda concorrente
} Args;
```

## Funzione del Worker

```c
void Worker(Args *a) {
    // --- Fase 1: solo il Worker 0 crea le partizioni ---
    if (a->id == 0) {
        for (i = 0; i < a->Q_parts; i++) {
            Task t = { bound(i), bound(i + 1) - 1 };   // bound(i) = i*(N/Q_parts) + min(i, N%Q_parts)
            if (t.start <= t.end) push(a->Q, t);       // salta le partizioni vuote (N < Q_parts)
        }
        close(a->Q);
    }

    // --- Fase 2: tutti ordinano i task prelevati dalla coda ---
    Task t;
    while (pop(a->Q, &t))                               // finché Q è vuota e chiusa
        qsort(&a->A[t.start], t.end - t.start + 1, sizeof(int), cmp);

    barrier();                                          // tutte le partizioni sono ordinate

    // --- Fase 3: merge in ceil(log2 Q_parts) passi sincroni ---
    int *src = a->A, *dst = a->B;
    for (k = 0, span = 1; span < a->Q_parts; k++, span *= 2) {
        // span = numero di partizioni in ciascun blocco; al passo k ci sono ceil(Q_parts/span)
        // blocchi, da fondere a coppie (l'eventuale ultimo blocco spaiato viene solo copiato)
        coppie = ceil(ceil(Q_parts / span) / 2);
        for (c = a->id; c < coppie; c += a->P) {        // con Q_parts = P: lavorano i primi P/2^(k+1)
            lo  = bound(2*c*span);                      // inizio blocco sinistro
            mid = bound(min(2*c*span + span,   Q_parts));   // inizio blocco destro
            hi  = bound(min(2*c*span + 2*span, Q_parts));   // fine del blocco destro (esclusa)
            merge(src, dst, lo, mid, hi);               // fonde src[lo..mid) e src[mid..hi) in dst[lo..hi)
        }
        barrier();                                      // fine passo: gli altri Worker attendono qui
        swap(src, dst);                                 // il risultato del passo diventa l'ingresso del successivo
    }
    // Il vettore ordinato è in src (A se i passi sono pari, B se dispari).
}
```

### Perché non servono mutex nel merge

- Al passo *k* ogni coppia `c` scrive solo `dst[lo..hi)`, intervalli **disgiunti** per coppie diverse.
- `src` è in sola lettura nello stesso passo.
- La `barrier()` di fine passo separa le scritture del passo *k* dalle letture del passo *k+1*: i buffer
  scambiano ruolo solo dopo che tutti i Worker hanno finito.
