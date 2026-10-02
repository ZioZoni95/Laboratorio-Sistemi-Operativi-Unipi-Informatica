# Esercitazioni di laboratorio

Compilazione generica (dalla cartella dell'esercizio): `gcc -Wall -Wextra -pthread file.c -o file`.
Dove serve altro, il comando è indicato in tabella.

| Cartella | Argomento | Esercizi |
|---|---|---|
| `esercitazione-01` | Stringhe, funzioni variadiche, parsing manuale di `argv` | `assignment1_1.c` `strtoupper` · `assignment1_2.c` `mystrcat` (varargs) · `assignment1_3.c` parser di opzioni senza `getopt` |
| `esercitazione-02` | `getopt`, puntatori a funzione, `strtok_r`, `rand_r` | `assignment2_1.c` getopt · `2_2` puntatori a funzione · `2_3` tokenizzazione con `strtok_r` · `2_4` numeri casuali con `rand_r` e istogramma delle occorrenze |
| `esercitazione-03` | Librerie statiche | `libtokenizer.a` con `tokenizer` / `tokenizer_r` (`src/`, `inc/`, `main/`) |
| `esercitazione-04` | I/O su file | `4_1` estrae i login name da `/etc/passwd` · `4_2` matrice `float` salvata in binario e testo, confronto con `memcmp` |
| `esercitazione-05` | File system (`stat`, `opendir`, ...) | `es1` clone di `cp` (+ versione ottimizzata, grafici dei tempi `.jpg`) · `es2` `myfind` · `es3` permessi di un file · `es4` `lsdir` ricorsivo |
| `esercitazione-06` | Processi (`fork`, `exec`, `wait`) | `es1` `dummyshell` (+ ottimizzata) · `es2` esecuzione in background · `es3` processi zombie · `es4` albero di processi · `es5` Fibonacci con un processo per chiamata. Ha un `Makefile`: `make` |
| `esercitazione-07` | Thread e sincronizzazione | `es1` produttori/consumatori con buffer limitato · `es2` filosofi a cena (3 versioni) · `es3` pipeline a 3 thread con FIFO illimitata |
| `esercitazione-08` | Coda concorrente e pipe | `es1` produttori/consumatori su coda (K messaggi, M produttori, N consumatori) · `es2` calcolatrice che delega a `bc -lq` (processo figlio via `fork`/`execlp`) comunicando con due pipe · `soluzione_prof` |
| `lettori-scrittori` | Problema lettori/scrittori con coda FIFO | `RW_fifo.c` + `include/queue.{c,h}` |

## Comandi di compilazione non banali

```bash
# Esercitazione 3 – libreria statica
cd esercitazione-03
gcc -Wall -c src/tokenizer_lib.c -o src/tokenizer_lib.o
mkdir -p lib && ar rcs lib/libtokenizer.a src/tokenizer_lib.o
gcc -Wall main/tokenizer_main.c -Llib -ltokenizer -o main/tokenizer_test

# Esercitazione 6 – tutti gli esercizi (con AddressSanitizer)
cd esercitazione-06 && make

# Esercitazione 7
cd esercitazione-07/es1 && gcc -Wall -pthread prod_cons_limitbuff.c wrappers.c -o prod_cons
cd esercitazione-07/es2 && gcc -Wall -pthread -I../es1 filosofi.c ../es1/wrappers.c -o filosofi   # usa i wrapper di es1
cd esercitazione-07/es3 && gcc -Wall -pthread main.c unbounded_fifo.c wrappers.c -o pipeline

# Esercitazione 8
cd esercitazione-08/es1 && gcc -Wall -pthread -Iinclude main.c include/queue.c -o prodcons
```

> Alcuni esercizi hanno problemi noti (es. `esercitazione-03` produce warning di funzioni non dichiarate,
> `esercitazione-05/es3` e `es2` hanno bug). Dettagli in [`../docs/ANALISI_CODICE.md`](../docs/ANALISI_CODICE.md).
