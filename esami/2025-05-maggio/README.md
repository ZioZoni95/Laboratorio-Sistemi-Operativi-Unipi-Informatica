# Ordinamento parallelo con coda concorrente — Appello del 12 maggio 2025

Traccia: [`Appello-Maggio.pdf`](Appello-Maggio.pdf) (AESO, vecchio ordinamento).

Il programma ordina un vettore di `N` interi casuali con `P` thread **Worker** (`P` potenza di 2) che
collaborano tramite una **coda concorrente** di task e una **barriera**.

## Uso

```bash
make                          # compila → ./parallel_sort
./parallel_sort -n <N> -w <P> # N = numero di elementi (>0), P = numero di worker (potenza di 2)
make test                     # batteria di test (test.sh), log in log/
make clean
```

Esempio:

```
$ ./parallel_sort -n 30 -w 4
...
Verifica: L'array è ordinato correttamente.
```

| Opzione | Significato | Vincoli |
|---|---|---|
| `-n` | numero di elementi dell'array | intero positivo |
| `-w` | numero di thread Worker | intero positivo, potenza di 2 (1, 2, 4, 8, …); altrimenti errore ed `exit 1` |

## Algoritmo

1. **Partizionamento** — il Worker 0 divide l'array in `P` partizioni contigue (le prime `N mod P` hanno un
   elemento in più), inserisce i task `(start, end)` nella coda e la **chiude** (`close_queue`).
2. **Sorting** — ogni Worker fa `pop` di un task alla volta e ordina la partizione con `qsort`, finché la
   coda è vuota **e** chiusa.
3. **Barriera** (`pthread_barrier_wait`) — tutte le partizioni sono ordinate.
4. **Merge in log₂P passi** — al passo *k* sono attivi i primi `P / 2^(k+1)` Worker; ciascuno fonde due
   blocchi adiacenti (`merge_sections`) da `array` a `temp_array`, poi i risultati sono ricopiati in
   `array`. Una barriera separa merge e copia e un'altra chiude il passo.
5. Il `main` verifica che l'array finale sia ordinato e libera le risorse.

Se `N < P` alcune partizioni sono vuote e vengono saltate: i Worker senza lavoro partecipano comunque a
tutte le barriere.

## Struttura

```
esami/2025-05-maggio/
├── Makefile
├── test.sh              batteria di test (P = 1, 2, 4; N < P, N = P, N > P, N non multiplo di P, stress)
├── include/
│   ├── common.h         macro di errore/debug, struct Task e ThreadArgs
│   ├── queue.h          coda concorrente (lista + mutex + variabile di condizione)
│   ├── worker.h
│   └── myutils.h        qsort_compare, merge_sections, print_array
└── src/
    ├── main.c           parsing opzioni, allocazione, creazione/join dei thread, verifica
    ├── worker.c         funzione eseguita dai Worker (fasi 1-4)
    ├── queue.c          init/destroy/push/pop/close_queue
    └── myutils.c
```

## Debug

`include/common.h` definisce `DEBUG` (default `0`). **Attenzione:** con `DEBUG 0` il programma stampa le
tracce *essenziali* di partizioni e merge (`[INDICI SORTING]`, `[INDICI MERGING]`, `[WORKER STATUS]`);
con `DEBUG 1` stampa tracce molto dettagliate di ogni passo. Per i controlli dinamici:

```bash
gcc -g -O1 -fsanitize=thread  -pthread -Iinclude src/*.c -o /tmp/ps_tsan && /tmp/ps_tsan -w 8 -n 100000
gcc -g -O1 -fsanitize=address,undefined -pthread -Iinclude src/*.c -o /tmp/ps_asan && /tmp/ps_asan -w 16 -n 100001
```

Su questa versione entrambi risultano senza segnalazioni.

## Problemi noti e miglioramenti

Vedi [`../../docs/ANALISI_CODICE.md`](../../docs/ANALISI_CODICE.md#2-progetto-principale-esami2025-05-maggio):
tra gli altri, i due mutex attorno a merge e copia sono superflui, e l'ultimo test di `test.sh` risulta
`[ERRORE]` per un difetto dello script (non del programma).
