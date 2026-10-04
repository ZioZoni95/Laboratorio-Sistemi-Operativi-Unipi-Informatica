# Analisi del codice: problemi e aree di miglioramento

Questa analisi accompagna la riorganizzazione della repository. I problemi di `esami/2025-05-maggio` sono
stati **corretti** (§2); **il codice di tutti gli altri progetti non è stato modificato** (a parte i tre
script di test di Settembre 2024, vedi §1): per quelli sono elencati i problemi trovati, così da poter
decidere cosa correggere.

## 0. Cosa è stato verificato (e cosa no)

| Verifica | Ambito |
|---|---|
| `gcc 13.3 -Wall -Wextra -fsyntax-only` | tutti i 72 file `.c` |
| Build + esecuzione, ThreadSanitizer, ASan/UBSan, misure di tempo, controllo di mutazione dei test | `esami/2025-05-maggio` (prima e dopo le correzioni) |
| Build + esecuzione di casi limite | `esami/2024-09-settembre/versione-1` (`R=0`, `R=abc`), `esami/2025-03-marzo/new_ProgettoMarzo25` (ASan, vari N/P/k/C) |
| Build con il Makefile | `2023-07-luglio`, `2024-09-settembre/*`, `2025-03-marzo/*` |
| Compilazione e link dei comandi riportati nei README | `esercitazione-03`, `-05/es1`, `-06` (dry-run), `-07`, `-08` |

**Non verificato:** il comportamento a runtime della maggior parte delle esercitazioni, i progetti
`2025-03-marzo/Progetto_marzo` e `progetto_marzo_gem` oltre al link, il contenuto degli archivi `.tgz`.
Le segnalazioni su codice non eseguito (es. `lstat`, `opendir`) derivano dalla lettura del sorgente e dai
warning del compilatore.

Legenda: **🔴 bug / difetto confermato** · **🟠 rischio o fragilità** · **🟡 miglioramento**.

---

## 1. Igiene della repository

| | Problema | Stato |
|---|---|---|
| 🔴 | **44 file binari/IDE versionati**: eseguibili ELF, `.o`, `libtokenizer.a`, cartelle `.idea/` e `.vscode/`. Il commit `rimossi file eseguibili` non li aveva tolti tutti e ogni `make` li sporcava (`modified: obj/*.o`). | **Risolto**: tolti dal tracking (restano su disco, ignorati). |
| 🔴 | Nessun `.gitignore`. | **Risolto**: aggiunto. |
| 🟠 | Le configurazioni IDE contenevano **path assoluti personali** (`/home/antoninoc/...` in `launch.json`). | Tolti dal tracking, ma **restano nella cronologia git**. |
| 🟠 | Il peso dei binari resta nella cronologia. | **Non fatto**: richiede di riscrivere la storia (`git filter-repo`), operazione distruttiva da decidere a parte. |
| 🔴 | Gli script `tests/{pre_finaltest,reduced_test,final_test}.sh` di Settembre 2024 usavano il path `../Esame_Settembre24/output/log.txt` (funzionava solo se la cartella si chiamava così) e facevano `cd ../` assumendo di essere lanciati da `tests/`. | **Risolto**: path calcolato dalla posizione dello script, `mkdir -p output`. Verificato con `reduced_test.sh`. |
| 🟡 | Nomi incoerenti e refusi: `Esercizi Lab` (spazio), `LettScritt`, `thread_exeample.c`, `deabug_esame`, mix IT/EN. | Cartelle rinominate in kebab-case; file lasciati invariati. |
| 🟡 | Nessuna `LICENSE`; gli archivi `soluzioni-didawiki/*.tgz` sono materiale del corso (e non sono ispezionabili dalla UI GitHub). | Da decidere. |

---

## 2. Progetto principale: `esami/2025-05-maggio` — **risolto**

Questa sezione elencava i difetti del progetto di maggio. **Sono stati corretti**: il dettaglio, le misure
prima/dopo e ciò che resta aperto sono nel [README del progetto](../esami/2025-05-maggio/README.md#5-cosa-è-stato-migliorato).
Riepilogo dei problemi originali:

| | Problema originale | Esito |
|---|---|---|
| 🔴 | `test.sh`: lo stress test risultava sempre `[ERRORE]` (`time` dentro una variabile), `exit 0` incondizionato, ramo "Errore Atteso" mai usato, copertura minima | Riscritto: 142 test, exit code corretto, timeout, `make check` con ASan/UBSan e TSan. Verificato con un controllo di mutazione (4 bug su 4 rilevati) |
| 🟠 | `merge_mutex`/`copy_mutex` serializzavano il merge; `__sync_synchronize` superfluo | Rimossi (TSan pulito). Merge con buffer alternati, una barriera per passo |
| 🟠 | `P` solo potenza di 2; partizioni fisse a `P` | Qualunque `P ≥ 1`; opzione `-p` per le partizioni |
| 🟠 | `atol`/`atoi` senza controlli; indici `int` con `N` `long`; exit code `0` con risultato errato; `pthread_*` non controllati | `getopt` + `strtol`, tipi `long`, exit code `1`, verifica di permutazione (checksum, esatta con `-c`), ogni chiamata controllata |
| 🟡 | Semantica di `DEBUG` invertita; `worker_thread` da 340 righe; indici di merge in `O(P)`; `task_count` inutilizzato; Makefile senza `-O2`/sanitizer | `-v` a runtime; fasi separate; formula chiusa `O(1)`; rimosso; target `test-asan`/`test-tsan`/`check` |

Prestazioni (4 core, `N = 40 M`, fase di ordinamento): P=2 −4 %, P=4 −13 %. Non ancora affrontato: l'ultimo
passo di merge resta su un solo Worker (limite della traccia; vedi gli sviluppi futuri nel README).

---

## 3. Altri progetti d'esame

### `2025-03-marzo`
- 🔴 **`progetto_marzo_gem`: il Makefile non produce l'eseguibile.** `wildcard src/*.c` compila insieme
  `main.c`+`main2.c`, `master.c`+`master2.c`, ecc., che sono **due implementazioni distinte** con gli stessi
  simboli globali (`V`, `N`, `k`, `main`…): `ld` segnala `multiple definition`. Servono due directory o
  due target.
- 🔴 **`new_ProgettoMarzo25`: `k = 0` fa bloccare il programma** (verificato: restano attivi finché non
  scade il timeout). Nessuna validazione di `numWorkers`, `N`, `k`, `C` (`main.c:26-29`, tutti `atoi`).
- 🟠 `new_ProgettoMarzo25/src/main.c:21`: il messaggio d'uso termina con `\\n` (backslash + `n` stampati
  letteralmente, manca l'a capo).
- 🟡 La somma è corretta in tutte le prove (N = 7 … 100 000, P = 1…4, C = 1…8) e ASan/UBSan sono puliti.
- 🟡 `Progetto_marzo`: nessun Makefile; `main.c`/`main_deb.c` e `threads.c`/`threads_debug.c` sono copie
  parallele (variante debug) invece di una macro; `pseudo.c` è pseudocodice (non compila, 24 errori) e
  andrebbe etichettato come tale (es. `pseudo.c.txt`).
- 🟡 Tre implementazioni dello stesso esercizio: tenerne una, le altre in un branch o in `archivio/`.

### `2024-09-settembre`
- 🔴 **`versione-1`: un valore non valido blocca il programma.** `src/main.c:27-30` stampa "R deve essere un
  numero positivo" ma `exit(1)` è commentato (`main.c:12`, `print_usage`). Verificato: `R=0` e `R=abc`
  non terminano (kill dopo 10 s).
- 🔴 `versione-1/src/main copy.c` **non compila** (`francesco`, `federico`, `franco` non dichiarate): file
  residuo, da eliminare.
- 🟠 `versione-2/main.c:36`: `usleep` non dichiarata con `-std=c11` (serve `_DEFAULT_SOURCE`). Oggi è un
  warning; con gcc ≥ 14 gli errori di dichiarazione implicita sono bloccanti. `-lpthread` è in
  `CFLAGS` invece che in `LDFLAGS`/`LDLIBS`.
- 🟡 `#include <queue.h>` con parentesi angolari: funziona solo grazie a `-I`; per header di progetto
  è più chiaro `"queue.h"`.
- 🟡 Due versioni quasi equivalenti con test diversi (`tests/*.sh` vs `test.sh`).

### `2023-07-luglio`
- Compila senza warning. 🟡 `sync.h` chiude con `//SYNC_C` (refuso); variabili globali condivise
  (`extern`) senza prefisso: rischio di collisione di nomi (`wait` ha lo stesso nome della funzione di
  libreria `wait(2)`).

---

## 4. Esercitazioni

| | File | Problema |
|---|---|---|
| 🔴 | `esercitazione-03/inc/tokenizer.h:1` | `#ifdef TOKENIZER_H` invece di `#ifndef`: **il corpo dell'header non viene mai incluso**, le dichiarazioni mancano e `tokenizer_main.c:22,32` produce `implicit declaration`. Oggi è un warning; con gcc ≥ 14 è un **errore**. |
| 🔴 | `esercitazione-05/es3/assignment5_3.c:47` | `mode[0] = "l";` assegna un `char *` a un `char` (warning `-Wint-conversion`): carattere di tipo link errato. Inoltre al `:38` usa `stat` e non `lstat`, quindi `S_ISLNK` non è mai vero per un link simbolico. |
| 🔴 | `esercitazione-05/es2/assignment5_2.c:32,51` | `SYSCALL(opendir, …)` e `SYSCALL(realpath, …)` confrontano il risultato con `-1` ma entrambe le funzioni restituiscono `NULL` in errore: un `opendir` fallita non viene rilevata e si dereferenzia `NULL`. Segnalato anche da `-Wextra` (`utils.h:23`, confronto puntatore/intero). |
| 🟠 | `esercitazione-05/*/utils.h:41` | La macro `FOPEN` fa `return 1;` ma è usata in funzioni `void` (warning in `assignment5_1_optimized.c`). |
| 🟠 | `lettori-scrittori/include/queue.c:208` | `q_len` è `unsigned long` (`queue.h:19`): `assert(q->q_len >= 0)` è sempre vero e l'underflow non è rilevato (`-Wtype-limits`). |
| 🟠 | `librerie/unboundedfifo.h` | Include `wrappers.h`, che esiste solo in `esercitazione-07/es1` e `es3`: la libreria **non si compila da sola**. |
| 🟠 | `esempi-lezione/thread_exeample.c` | Mancano `#include <stdio.h>/<unistd.h>` (7 warning), la funzione del thread non ha `return`, assegnazione in condizione (`:16`). |
| 🟡 | `esercitazione-07/es2/*.c` | Includono `wrappers.h` ma non lo contengono: compilano solo con `-I../es1` (documentato nel README). |
| 🟡 | vari | Warning `unused parameter 'arg'` nelle funzioni dei thread: `(void)arg;`. |
| 🟡 | `esercitazione-08/es1/main.c` | `-1` come sentinella di terminazione dei consumatori: oggi i valori sono sempre ≥ 0 (`id*100+i`) quindi funziona, ma è fragile se cambia il formato dei messaggi. |

### Codice duplicato (identico byte per byte)

| File | Copie |
|---|---|
| `esercitazione-05/es{1,2,3,4}/utils.h` | 4 |
| `esercitazione-06/es{1,2,3,4}/utils.h` | 4 |
| `esercitazione-07/es{1,3}/wrappers.{c,h}` | 2 + 2 |
| `esercitazione-08/{es2,soluzione_prof}/utilities.h` | 2 |
| `lettori-scrittori/include/queue.c` = `esami/2024-09-settembre/versione-2/include/queue.c` | 2 |

Una modifica (come la correzione di `FOPEN`/`SYSCALL` qui sopra) andrebbe oggi replicata a mano in
tutte le copie. Soluzione: un `common/` per esercitazione con `-Icommon`.

---

## 5. Miglioramenti trasversali

1. **Script o Makefile radice** (`make all` / `make check`) che compili ogni progetto con
   `-Wall -Wextra -Werror` e fallisca se qualcosa non compila: oggi diversi file producono warning o non
   si compilano da soli (vedi §3-4).
2. **GitHub Actions**: compilazione con gcc e clang, `make test` per `2025-05-maggio`, e una run con
   ThreadSanitizer sui progetti a thread. Richiede prima di correggere l'`exit 0` di `test.sh`.
3. **Convenzione per i test**: ogni progetto con `make test` che restituisce un exit code significativo.
4. **Un solo `utils.h`/`wrappers.h` condiviso** (vedi duplicati) e `-pthread` coerente nei Makefile.
5. **Archiviare le varianti** (tre implementazioni di marzo, due di settembre) lasciandone una per esame.

## 6. Priorità consigliata

| Priorità | Intervento | Sforzo |
|---|---|---|
| ✅ | Maggio: `test.sh`, mutex, `P` qualsiasi, validazione degli argomenti, ping-pong, refactor (§2) | fatto |
| 1 | `tokenizer.h` (`#ifndef`), `assignment5_3.c:47` e `lstat`, `opendir`/`realpath` in `5_2` | minimo |
| 2 | `versione-1` di settembre: `exit(1)` su valori non validi; eliminare `main copy.c` | minimo |
| 3 | `progetto_marzo_gem`: separare le due implementazioni | basso |
| 4 | Maggio: merge parallelo con co-ranking, barriera scritta a mano, input non casuali (vedi gli sviluppi futuri nel README) | medio |
| 5 | CI e Makefile radice | medio |
| 6 | Riscrittura della storia per eliminare i binari dalla cronologia | decisione a parte |
