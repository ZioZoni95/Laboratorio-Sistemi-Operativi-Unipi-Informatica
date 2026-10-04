# Ordinamento parallelo con coda concorrente

Mini-progetto del **Primo Appello — 12 maggio 2025** (AESO, corso A e B, vecchio ordinamento, A.A. 2024/2025).

📄 **Traccia:** [`Appello-Maggio.pdf`](Appello-Maggio.pdf)

Il programma ordina un vettore di `N` interi con `P` thread **Worker** che collaborano tramite una
**coda concorrente** di task e una **barriera**.

## Indice

1. [La traccia in breve](#1-la-traccia-in-breve)
2. [Uso](#2-uso)
3. [Come funziona](#3-come-funziona)
4. [Struttura del progetto](#4-struttura-del-progetto)
5. [Analisi del codice](#5-analisi-del-codice)
6. [Aree di miglioramento](#6-aree-di-miglioramento)
7. [Sviluppi futuri](#7-sviluppi-futuri)
8. [Come è stato verificato](#8-come-è-stato-verificato)

---

## 1. La traccia in breve

| Requisito del PDF | Dove è realizzato |
|---|---|
| `P ≥ 1` Worker numerati da `0` a `P−1` | `main.c` crea i thread, `thread_id` in `ThreadArgs` |
| Il Worker 0 calcola `P` partizioni disgiunte e fa `push` di `(start,end)` nella coda `Q` | `worker.c`, fase 1 |
| Tutti i Worker fanno `pop` e ordinano la partizione con `qsort` finché `Q` è vuota | `worker.c`, fase 2 |
| Barriera: tutte le partizioni sono ordinate prima del merge | `pthread_barrier_wait`, `worker.c:141` |
| Merge in `log₂P` passi sincroni; al passo *k* lavorano i primi `P/2^(k+1)` Worker, gli altri attendono in barriera | `worker.c`, fase 3 |
| Coda con `push`/`pop`, sincronizzata con mutex e variabili di condizione | `queue.c` (lista + `pthread_mutex_t` + `pthread_cond_t`) |
| Barriera (si può usare `pthread_barrier_wait`) | `pthread_barrier_t` |
| Opzioni: numero di Worker (≥ 1) e dimensione `N` | `-w` e `-n` |
| `Makefile` per compilare ed eseguire i test | `Makefile`, `test.sh` |
| Pseudocodice C di tipi di dato e funzione del Worker | **non presente nella repository** (vedi §6.1) |

> **Nota sul PDF.** Il testo contiene due refusi ereditati dall'appello di marzo: parla di elementi "da
> sommare" (in realtà si ordinano) e di consegna "entro il 13 Marzo". Non influiscono sull'esercizio.

---

## 2. Uso

```bash
make                          # compila → ./parallel_sort
./parallel_sort -n <N> -w <P> # N = numero di elementi, P = numero di Worker
make test                     # batteria di test (test.sh), output in log/
make clean
```

| Opzione | Significato | Vincoli attuali |
|---|---|---|
| `-n` | numero di elementi dell'array (valori casuali) | intero positivo |
| `-w` | numero di thread Worker | intero positivo **e potenza di 2** (1, 2, 4, 8, …); altrimenti messaggio d'errore ed `exit 1` |

```
$ ./parallel_sort -n 30 -w 4
Avvio parallel_sort con N=30 elementi e P=4 worker (da -w).
...
Verifica: L'array è ordinato correttamente.
```

---

## 3. Come funziona

```
Worker 0                    Worker 1..P-1
   │ calcola le P partizioni      │
   │ push(start,end) × P          │            ┌─────────────── Coda Q ───────────────┐
   │ close_queue()                │            │ (0,7) (8,15) (16,22) (23,29)   closed │
   ▼                              ▼            └───────────────────────────────────────┘
   └──── pop → qsort(partizione) finché Q è vuota e chiusa ────┘
                          │
                  ═══ BARRIERA 1 ═══   (tutte le partizioni sono ordinate)
                          │
   passo 0:  W0 fonde [0-7]+[8-15]        W1 fonde [16-22]+[23-29]     (W2, W3 inattivi)
                  ═══ barriera ═══  copia temp_array → array  ═══ barriera ═══
   passo 1:  W0 fonde [0-15]+[16-29]                                    (W1, W2, W3 inattivi)
                  ═══ barriera ═══  copia temp_array → array  ═══ barriera ═══
                          │
                    array ordinato   (esempio per N = 30, P = 4)
```

1. **Partizionamento** — il Worker 0 divide l'array in `P` blocchi contigui: le prime `N mod P` partizioni
   hanno un elemento in più. Inserisce i task nella coda e la **chiude** con `close_queue`.
2. **Sorting** — `pop` si blocca su una variabile di condizione finché c'è un task o la coda è chiusa;
   restituisce `0` solo quando la coda è **vuota e chiusa**. È ciò che permette ai Worker di terminare
   senza sapere in anticipo quanti task ci sono.
3. **Merge a passi sincroni** — al passo *k* sono attivi i primi `P/2^(k+1)` Worker; ciascuno ricava dai
   propri indici i due blocchi adiacenti, li fonde con `merge_sections` in `temp_array`, e dopo una barriera
   li ricopia in `array`. Una seconda barriera chiude il passo.
4. **Casi limite** — se `N < P` alcune partizioni sono vuote e non vengono accodate, ma i Worker senza
   lavoro partecipano comunque a tutte le barriere (altrimenti si avrebbe un deadlock).
5. Il `main` verifica che il risultato sia ordinato, stampa l'esito e libera le risorse.

---

## 4. Struttura del progetto

```
esami/2025-05-maggio/
├── Appello-Maggio.pdf   traccia dell'appello
├── README.md
├── Makefile             all · test · clean
├── test.sh              batteria di test (P = 1, 2, 4; N < P, N = P, N > P, N non multiplo di P, stress)
├── include/
│   ├── common.h         macro di errore e debug, struct Partition_Index_Task e ThreadArgs
│   ├── queue.h          ConcurrentQueue: lista + mutex + variabile di condizione + flag `closed`
│   ├── worker.h         prototipo di worker_thread
│   └── myutils.h        qsort_compare, merge_sections, print_array
└── src/
    ├── main.c           parsing opzioni, allocazione, creazione/join dei thread, verifica, cleanup
    ├── worker.c         funzione eseguita dai Worker (fasi 1–3)
    ├── queue.c          init_queue / destroy_queue / push / pop / close_queue
    └── myutils.c
```

Circa 1000 righe di C in `src/` (`worker.c` 373, `myutils.c` 243, `main.c` 203, `queue.c` 203) più `test.sh` (95).

---

## 5. Analisi del codice

### Punti di forza

- **Correttezza solida**: ordina correttamente in tutti i casi provati (§8) e i sanitizer non segnalano nulla.
- **La coda è ben progettata**: `closed` + `pthread_cond_broadcast` è il modo giusto di far terminare i
  consumatori; `pop` ricontrolla la condizione in un `while` (gestisce i *spurious wakeup*) e libera il
  nodo fuori dalla sezione critica.
- **Gestione dei casi limite**: `N < P`, `N` non multiplo di `P`, `P = 1` (nessun passo di merge).
- **Modularità**: coda, utilità di merge e logica dei Worker sono in file separati con header chiari.
- **Compila senza warning** con `-Wall -Wextra`; errori di sistema gestiti dalle macro `CHECK_ERR` e
  `CHECK_PTHREAD_ERR`.
- **Verifica finale integrata** nel programma e `merge_sections` protetta da `assert` sugli indici.

### Profilo delle prestazioni

Misurato su una macchina a 4 core, compilando con `-O2`, `N = 40 000 000`, una sola esecuzione per
configurazione (i valori sono indicativi). Il tempo è suddiviso fra la fase di sorting (inclusa la
partenza dei thread) e la fase di merge:

| P | sorting | merge | totale | speedup |
|---|---|---|---|---|
| 1 | 7,67 s | — | 7,7 s | 1× |
| 2 | 3,66 s | 0,48 s | 4,1 s | ≈ 1,9× |
| 4 | 1,75 s | 0,88 s | 2,6 s | ≈ 2,9× |

- Il **sorting scala bene** (anche oltre il lineare, perché partizioni più piccole stanno meglio in cache e
  `qsort` costa `O(n log n)`).
- Il **merge non scala**: con P = 4 vale circa **un terzo** del tempo. Il motivo è strutturale: l'ultimo
  passo fonde tutti gli `N` elementi su **un solo Worker** mentre gli altri attendono in barriera
  (legge di Amdahl). Questo limita lo speedup massimo indipendentemente da P.
- Anche i due `mutex` attorno a merge e copia (`worker.c:266-273`, `325-336`) fanno sì che i Worker attivi
  non fondano mai davvero in parallelo. Rimuoverli (e `__sync_synchronize`, `worker.c:279`) migliora il tempo
  totale dell'**8–13 %** con ThreadSanitizer ancora pulito, perché ogni Worker scrive un intervallo disgiunto
  e le barriere separano già le fasi.

---

## 6. Aree di miglioramento

Ordinate per priorità. Le voci marcate **[verificato]** sono state riprodotte eseguendo il codice.

### 6.1 Aderenza alla traccia

| | Aspetto | Dettaglio |
|---|---|---|
| 🟠 | **Pseudocodice mancante** | La traccia chiede, prima dell'implementazione, pseudocodice C dei tipi e della funzione del Worker. Nella repo c'è solo l'implementazione: si può aggiungere un `docs/pseudocodice.md` derivato da `worker.c`. |
| 🟠 | **"Al variare del numero di partizioni"** | Il testo richiede correttezza anche variando il numero di partizioni, ma il programma le fissa a `P` e non c'è un'opzione per cambiarle. Un'opzione `-p` (partizioni ≥ `P`) renderebbe la coda realmente utile come bilanciatore. |
| 🟠 | **`P` solo potenza di 2** (`main.c:60`) | La traccia dice `P ≥ 1`. `-w 3` o `-w 6` vengono rifiutati: scelta difendibile per i `log₂P` passi, ma un test con `P` qualsiasi fallirebbe. Il merge ad albero si estende con `⌈log₂P⌉` passi in cui un Worker senza "compagno" non fa nulla. |
| 🟡 | **Barriera propria** | La traccia ammette `pthread_barrier_wait`, ma parla di sincronizzazione con mutex e variabili di condizione: una barriera riutilizzabile scritta a mano (contatore + generazione) mostrerebbe lo stesso schema usato in `queue.c`. |

### 6.2 Test

| | Problema | Dettaglio |
|---|---|---|
| 🔴 | **Lo stress test risulta sempre `[ERRORE]`** [verificato] | `test.sh:89` passa `"time $PROGRAM …"` e `run_test` lo esegue con `$command`: `time` è una parola riservata della shell e non viene riconosciuto, quindi il programma non parte. `make test` mostra `[OK]` per i primi 10 test e `[ERRORE]` per l'ultimo. |
| 🔴 | **`test.sh` termina sempre con `exit 0`** (riga 96) | Con test falliti restituisce comunque successo: inutilizzabile in una pipeline di CI. |
| 🟠 | Il ramo "Errore Atteso" **non è mai usato** | Esiste per `P` non potenza di 2, ma nessun test lo invoca. |
| 🟠 | Copertura limitata | Manca `P = 8, 16`, `N` grandi, argomenti non validi (`-n 0`, `-n abc`, `-w 0`), e un test ripetuto più volte per far emergere eventuali race. |
| 🟡 | `sleep 1` dopo ciascuno degli 11 test; esito dedotto da `grep` su una frase italiana | Meglio un exit code significativo (vedi 6.3). |

### 6.3 Robustezza

| | Problema | Dettaglio |
|---|---|---|
| 🟠 | **Il risultato errato non cambia l'exit code** (`main.c:180-184,203`) | Se la verifica fallisce il programma stampa l'errore ma restituisce `EXIT_SUCCESS`. Per questo `test.sh` deve cercare una stringa nell'output. |
| 🟠 | **`atol`/`atoi` senza controlli** (`main.c:38,41`) | `-n 12abc` viene letto come 12; overflow e valori fuori range non sono segnalati. Meglio `strtol` con `errno`/`endptr` e un limite massimo. |
| 🟠 | **Indici `int` con `N` `long`** (`common.h:49-50`, parametri di `merge_sections`) | Per `N > 2³¹−1` start/end traboccano. Usare `long`/`size_t` ovunque. |
| 🟡 | `rand() % (n*10)` (`main.c:82`) | `RAND_MAX` è 2³¹−1: per `N` grande i valori non sono uniformi. Va bene per un test, ma va saputo. |
| 🟡 | Valori di ritorno ignorati | `pthread_join` (`main.c:144`) e le chiamate `pthread_mutex_*`/`pthread_cond_*` di `queue.c`. |
| 🟡 | `task_count` mantenuto ma mai letto | Rimuoverlo o usarlo (es. in `print`/`assert`). |

### 6.4 Prestazioni

| | Intervento | Effetto atteso |
|---|---|---|
| 🟠 | **Togliere `merge_mutex` e `copy_mutex`** | Misurato: −8 % (P = 4) / −13 % (P = 2). Codice più semplice. |
| 🟠 | **Eliminare la fase di copia con due buffer alternati (*ping-pong*)** | Ad ogni passo si fonde da un buffer all'altro, senza ricopiare `temp_array → array`. Si elimina una barriera per passo e `O(N)` copie per passo (il passo finale copia tutti gli `N` elementi con un solo Worker). Alla fine, se serve, un solo `memcpy`. |
| 🟠 | **Merge parallelo degli ultimi passi (*merge path* / co-ranking)** | Ipotesi, **non misurata in questo repo**: spezzare ogni merge in `P` sottointervalli con una ricerca binaria (*co-rank*) sul punto di taglio, così che al passo finale lavorino tutti i Worker e non uno solo. Attacca direttamente il collo di bottiglia mostrato in §5. Va però **oltre il testo della traccia**, che prescrive di dimezzare i Worker attivi a ogni passo. |
| 🟡 | **Calcolo degli indici in `O(P)`** (`worker.c:191-210`) | Ogni Worker somma in un ciclo le dimensioni delle partizioni precedenti; basta la formula chiusa `start(i) = i·⌊N/P⌋ + min(i, N mod P)`, già implicita nella fase 1. |
| 🟡 | `memcpy` al posto del ciclo elemento per elemento (`worker.c:329-335`) | Più veloce e leggibile. |
| 🟡 | Makefile senza ottimizzazioni | Aggiungere `-O2` (o un target `release`) per i benchmark: `-g` senza `-O` falsa le misure. |

### 6.5 Qualità e manutenibilità

| | Problema | Dettaglio |
|---|---|---|
| 🟠 | **Semantica di `DEBUG` invertita** (`common.h:7`) | Con `DEBUG 0` (default) il programma **stampa** tracce di sorting e merge (`worker.c:50,80,109,238,368`, `main.c:87,150`); con `DEBUG 1` stampa tracce ancora più dettagliate. Meglio un livello di verbosità esplicito (`-v`) distinto da `DEBUG`, impostabile da riga di comando (`-DDEBUG=1`, con `#ifndef`). |
| 🟠 | **`worker_thread` è una funzione di ~340 righe** (`worker.c:35-373`) | Spezzarla in `phase_partition`, `phase_sort`, `phase_merge` (con un helper che calcola i blocchi del passo *k*). |
| 🟡 | Commenti molto verbosi che ripetono il codice (`i++; // Avanza l'indice`) | Meglio commentare il *perché* (invarianti, scelte di sincronizzazione). |
| 🟡 | Costanti numeriche e soglie di stampa sparse in `myutils.c` | Raccoglierle in `common.h`. |
| 🟡 | Il Makefile non ha target di analisi | Aggiungere `make asan` e `make tsan` (comandi in §8). |

---

## 7. Sviluppi futuri

Idee in ordine crescente di impegno, adatte a trasformare l'esercizio in un piccolo progetto di
approfondimento.

1. **Robustezza e CLI** — `getopt` con `-n`, `-w`, `-p` (partizioni), `-s` (seme), `-v` (verbose), `-h`;
   validazione con `strtol`; exit code coerente con l'esito della verifica.
2. **Test automatici seri** — `make test` che restituisce 0 solo se tutto passa; parametrizzare su
   `P ∈ {1,2,3,4,8,16}` e `N` da 0 a milioni; confronto con il risultato di `qsort` sequenziale; ripetizione
   dei test per scovare le race; esecuzione in CI con ThreadSanitizer.
3. **`P` arbitrario** — albero di merge con `⌈log₂P⌉` passi e Worker "spaiati" che non fondono; così la
   restrizione sulla potenza di 2 scompare.
4. **Più partizioni dei Worker** — con `partizioni > P` la coda bilancia davvero il carico quando i
   `qsort` hanno durate diverse (dati non uniformi, core eterogenei).
5. **Merge senza copia e ping-pong** — vedi §6.4.
6. **Merge parallelo con co-ranking** — misurare quanto migliora lo speedup rispetto al merge a Worker
   dimezzati della traccia; confrontare con una versione che segue il testo alla lettera.
7. **Sorting diverso da `qsort`** — `qsort` chiama una funzione di confronto per ogni coppia; un
   introsort con confronto inline o un *radix sort* sugli `int` riduce di molto il costo della fase 2.
8. **Generalizzare i tipi** — `size_t` per indici e dimensioni, dati letti da file o da `stdin`, comparatore
   passato come parametro e tipo degli elementi configurabile.
9. **Benchmark riproducibile** — script che varia `P` e `N`, ripete le misure, calcola media/deviazione e
   produce una tabella o un grafico (come già fatto per l'Esercitazione 5), con confronto con `qsort`
   sequenziale e con `sort -n`.
10. **Affinità e topologia** — `pthread_setaffinity_np` e attenzione a NUMA/hyper-threading: oltre i core
    fisici (es. `P = 8` su 4 core) il sorting non migliora più.
11. **Confronto con altri modelli** — la stessa pipeline con OpenMP (`#pragma omp task`) o con un *thread pool*
    persistente, per vedere cosa guadagna e cosa perde il codice rispetto alla versione POSIX esplicita.

---

## 8. Come è stato verificato

| Verifica | Esito |
|---|---|
| `make` con `-Wall -Wextra` | nessun warning |
| `make test` | 10 test `[OK]`, stress `[ERRORE]` per il difetto dello script (§6.2) |
| Casi manuali: `P = 1, 2, 4, 8, 16`; `N = 1, 3, 4, 30, 1000, 100 001, 250 001`; `N` fino a 40 000 000 | ordinato correttamente |
| ThreadSanitizer (`-fsanitize=thread`), P = 8, N = 100 000 | nessuna data race |
| AddressSanitizer + UBSan, P = 1, 8, 16 con N = 1, 3, 100 001 | nessun errore |
| Stessi controlli **senza** `merge_mutex`/`copy_mutex` | ThreadSanitizer ancora pulito |
| `-w 3` e `-n 0` | messaggio d'errore ed `exit 1` |

I controlli dinamici si riproducono con:

```bash
gcc -g -O1 -fsanitize=thread  -pthread -Iinclude src/*.c -o /tmp/ps_tsan && /tmp/ps_tsan -w 8 -n 100000
gcc -g -O1 -fsanitize=address,undefined -pthread -Iinclude src/*.c -o /tmp/ps_asan && /tmp/ps_asan -w 16 -n 100001
```

Le misure di tempo dipendono dalla macchina (qui 4 core): vanno ripetute sul proprio hardware prima di
trarre conclusioni. Il quadro generale dell'intera repository è in
[`../../docs/ANALISI_CODICE.md`](../../docs/ANALISI_CODICE.md).
