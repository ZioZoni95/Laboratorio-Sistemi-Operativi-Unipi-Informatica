# Ordinamento parallelo con coda concorrente

Mini-progetto del **Primo Appello — 12 maggio 2025** (AESO, corso A e B, vecchio ordinamento, A.A. 2024/2025).

📄 **Traccia:** [`Appello-Maggio.pdf`](Appello-Maggio.pdf) · ✍️ **Pseudocodice richiesto dalla traccia:** [`PSEUDOCODICE.md`](PSEUDOCODICE.md)

Il programma ordina un vettore di `N` interi con `P` thread **Worker** che collaborano tramite una
**coda concorrente** di task e una **barriera**.

## Indice

1. [La traccia in breve](#1-la-traccia-in-breve)
2. [Uso](#2-uso)
3. [Come funziona](#3-come-funziona)
4. [Struttura del progetto](#4-struttura-del-progetto)
5. [Cosa è stato migliorato](#5-cosa-è-stato-migliorato)
6. [Analisi delle prestazioni](#6-analisi-delle-prestazioni)
7. [Limiti noti e aree ancora aperte](#7-limiti-noti-e-aree-ancora-aperte)
8. [Sviluppi futuri](#8-sviluppi-futuri)
9. [Come è stato verificato](#9-come-è-stato-verificato)

---

## 1. La traccia in breve

| Requisito del PDF | Dove è realizzato |
|---|---|
| `P ≥ 1` Worker numerati da `0` a `P−1` | `main.c` crea i thread; qualunque `P ≥ 1` è accettato |
| Il Worker 0 calcola `P` partizioni disgiunte e fa `push` di `(start,end)` nella coda `Q` | `worker.c`, `phase_partition` |
| Tutti i Worker fanno `pop` e ordinano la partizione con `qsort` finché `Q` è vuota | `worker.c`, `phase_sort` |
| Barriera: tutte le partizioni sono ordinate prima del merge | `barrier_sync` in `worker_thread` |
| Merge in `log₂P` passi sincroni; al passo *k* lavorano i primi `P/2^(k+1)` Worker, gli altri attendono in barriera | `worker.c`, `phase_merge` |
| Correttezza "al variare del numero di Worker e **di partizioni**" | opzione `-p` (partizioni, default `P`) |
| Coda con `push`/`pop`, sincronizzata con mutex e variabili di condizione | `queue.c` |
| Barriera (si può usare `pthread_barrier_wait`) | `pthread_barrier_t` |
| Opzioni: numero di Worker (≥ 1) e dimensione `N` | `-w` e `-n` |
| `Makefile` per compilare ed eseguire i test | `Makefile`, `test.sh` |
| Pseudocodice C di tipi e funzione del Worker | [`PSEUDOCODICE.md`](PSEUDOCODICE.md) |

> **Nota sul PDF.** Il testo contiene due refusi ereditati dall'appello di marzo: parla di elementi "da
> sommare" (in realtà si ordinano) e di consegna "entro il 13 Marzo". Non influiscono sull'esercizio.

---

## 2. Uso

```bash
make                  # compila → ./parallel_sort  (-O2)
make test             # 142 test di correttezza (exit code 0 solo se passano tutti)
make check            # test normali + AddressSanitizer/UBSan + ThreadSanitizer
make clean
```

```
./parallel_sort -n <elementi> -w <worker> [-p <partizioni>] [-s <seme>] [-c] [-v] [-h]
```

| Opzione | Significato | Vincoli |
|---|---|---|
| `-n N` | numero di elementi dell'array (valori casuali) | obbligatoria, 1 … 2³¹−1 |
| `-w P` | numero di thread Worker | obbligatoria, 1 … 4096, **qualunque valore** (non solo potenze di 2) |
| `-p Q` | numero di partizioni iniziali | 1 … 2²⁰, default `P` |
| `-s S` | seme del generatore casuale | default: ora corrente (stampato, così il run è riproducibile) |
| `-c` | confronta il risultato con un `qsort` sequenziale (verifica esatta) | |
| `-v` | stampa le partizioni ordinate e i merge eseguiti da ogni Worker | |
| `-h` | mostra l'aiuto | |

Argomenti non validi (`-n 0`, `-n abc`, `-n 12abc`, `-w 0`, opzioni sconosciute, argomenti extra) producono
un messaggio d'errore ed `exit 1`. L'exit code è `0` **solo** se l'array finale è ordinato ed è una
permutazione di quello iniziale.

```
$ ./parallel_sort -n 30 -w 4 -s 1 -c -v
Avvio parallel_sort: N=30 elementi, P=4 worker, Q=4 partizioni (seme 1).
--- Array iniziale (N=30) ---
[283, 286, 177, 115, 293, 235, ...]
[Worker 0] inseriti 4 task, coda chiusa
[Worker 0] qsort partizione [0..7] (8 elementi)
...
[Worker 0] passo 0: fondo [0..8) + [8..16)
[Worker 1] passo 0: fondo [16..23) + [23..30)
[Worker 0] passo 1: fondo [0..16) + [16..30)
Ordinamento completato in 0.001 s.
Verifica: L'array è ordinato correttamente.
```

---

## 3. Come funziona

```
Worker 0                    Worker 1..P-1
   │ calcola le Q partizioni      │
   │ push(start,end) × Q          │            ┌─────────────── Coda Q ───────────────┐
   │ close_queue()                │            │ (0,7) (8,15) (16,22) (23,29)   closed │
   ▼                              ▼            └───────────────────────────────────────┘
   └──── pop → qsort(partizione) finché Q è vuota e chiusa ────┘
                          │
                  ═══ BARRIERA ═══   (tutte le partizioni sono ordinate)
                          │
   passo 0:  W0 fonde [0-7]+[8-15]        W1 fonde [16-22]+[23-29]     (W2, W3 inattivi)
                  array ──► temp_array              ═══ barriera ═══
   passo 1:  W0 fonde [0-15]+[16-29]                                    (W1, W2, W3 inattivi)
                  temp_array ──► array              ═══ barriera ═══
                          │
                    array ordinato   (esempio per N = 30, P = Q = 4)
```

1. **Partizionamento** — il Worker 0 divide l'array in `Q` blocchi contigui (le prime `N mod Q` partizioni
   hanno un elemento in più): la partizione *i* inizia in `i·⌊N/Q⌋ + min(i, N mod Q)`. Inserisce i task nella
   coda e la **chiude** con `close_queue`. Se `N < Q` le partizioni vuote non vengono accodate.
2. **Sorting** — `pop` si blocca su una variabile di condizione finché c'è un task o la coda è chiusa;
   restituisce `0` solo quando la coda è **vuota e chiusa**. Così i Worker terminano senza sapere in
   anticipo quanti task ci sono, e un Worker veloce prende più task di uno lento (bilanciamento dinamico).
3. **Merge a passi sincroni, con due buffer alternati (*ping-pong*)** — al passo *k* i blocchi sono
   `⌈Q/2^k⌉` e si fondono a coppie; il Worker *t* si occupa delle coppie *t*, *t+P*, *t+2P*…
   (con `Q = P` lavorano esattamente i primi `P/2^(k+1)` Worker, come richiesto). Ogni passo **legge da un
   buffer e scrive nell'altro**, poi i ruoli si scambiano. Non serve ricopiare i risultati né alcun mutex:
   ogni coppia scrive un intervallo disgiunto e la barriera di fine passo separa scritture e letture.
4. **`P` e `Q` qualsiasi** — se i blocchi a un passo sono dispari, l'ultimo resta senza compagno e viene
   semplicemente copiato nell'altro buffer; i passi sono `⌈log₂Q⌉`.
5. **Fine** — con un numero dispari di passi il risultato è in `temp_array`: il `main` lo ricopia in
   `array` (una sola `memcpy`). Poi verifica e libera le risorse.

Se `N < Q` i Worker senza lavoro partecipano comunque a tutte le barriere (altrimenti: deadlock).

---

## 4. Struttura del progetto

```
esami/2025-05-maggio/
├── Appello-Maggio.pdf   traccia dell'appello
├── PSEUDOCODICE.md      tipi e funzione del Worker in pseudocodice C (prima parte della consegna)
├── README.md
├── Makefile             all · test · test-asan · test-tsan · check · clean
├── test.sh              142 test: P × N, partizioni, argomenti non validi, ripetizioni, stress
├── include/
│   ├── common.h         macro (LOG, CHECK_ERR, CHECK_PTHREAD_ERR), Partition_Index_Task, ThreadArgs
│   ├── queue.h          ConcurrentQueue: lista + mutex + variabile di condizione + flag `closed`
│   ├── worker.h         prototipo di worker_thread
│   └── myutils.h        qsort_compare, partition_bound, merge_steps, merge_sections, print_array
└── src/
    ├── main.c           opzioni, generazione dati, avvio/join dei thread, verifica, pulizia
    ├── worker.c         worker_thread = phase_partition → phase_sort → barriera → phase_merge
    ├── queue.c          init/destroy/push/pop/close_queue
    └── myutils.c
```

435 righe in `src/` (prima ≈ 1020: `worker.c` da 372 a 107, `myutils.c` da 243 a 60, `queue.c` da 203 a 88, `main.c` da 203 a 180).

---

## 5. Cosa è stato migliorato

Rispetto alla versione precedente di questo progetto:

| Area | Prima | Ora |
|---|---|---|
| **Test** | lo stress test risultava sempre `[ERRORE]` (`time` dentro una variabile), `exit 0` incondizionato, 11 test | 142 test, exit code 1 se uno fallisce, timeout contro i deadlock, i fallimenti sono salvati in `log/`; `make test-asan` / `test-tsan` / `check` |
| **Merge** | 2 mutex (merge e copia) che serializzavano i Worker, 2 barriere + copia elemento per elemento a ogni passo | nessun mutex, **1 barriera per passo**, buffer alternati; una sola `memcpy` finale se i passi sono dispari |
| **Numero di Worker** | solo potenze di 2 | **qualunque `P ≥ 1`** |
| **Partizioni** | fisse a `P` | opzione `-p`, con più partizioni dei Worker la coda bilancia davvero il carico |
| **Argomenti** | `atol`/`atoi` (`-n 12abc` valeva 12), nessun limite | `getopt` + `strtol` con controllo di formato e intervallo, messaggio d'errore ed `exit 1` |
| **Esito** | un risultato errato stampava l'errore ma usciva con `0` | exit code `1`; verifica di ordinamento **e** di permutazione (somma + XOR), esatta con `-c` |
| **Indici** | `int` per start/end con `N` `long` | `long` ovunque |
| **Debug** | `DEBUG 0` *stampava* le tracce, `DEBUG 1` ne stampava di più | output silenzioso di default, `-v` a runtime per le tracce |
| **Struttura** | `worker_thread` di 340 righe, indici di merge calcolati con cicli `O(P)` | `phase_partition` / `phase_sort` / `phase_merge`, formula chiusa `O(1)` (`partition_bound`) |
| **Coda** | `task_count` mantenuto ma mai letto, valori di ritorno `pthread_*` ignorati | rimosso, ogni chiamata controllata (`CHECK_PTHREAD_ERR`) |
| **Makefile** | senza `-O2`, nessun controllo dinamico | `-O2`, `-std=gnu11`, target per ASan/UBSan e TSan |
| **Documentazione** | — | questo README e `PSEUDOCODICE.md` |

---

## 6. Analisi delle prestazioni

Macchina a **4 core**, `-O2`, `N = 40 000 000`. Tempo della sola fase di ordinamento (partenza dei thread,
sorting e merge; esclusi generazione dati e verifica), **minimo di 4 esecuzioni**:

| P | versione precedente | ora | differenza | speedup (ora) |
|---|---|---|---|---|
| 1 | 6,72 s | 6,67 s | −1 % | 1× |
| 2 | 3,59 s | 3,43 s | −4 % | 1,9× |
| 4 | 2,19 s | 1,90 s | **−13 %** | 3,5× |

Alcune osservazioni:

- Il **sorting scala bene**: partizioni più piccole stanno meglio in cache e `qsort` costa `O(n log n)`.
- Il guadagno viene da meno sincronizzazione e meno copie: con `P = 4` mancano un mutex per fusione, un mutex
  per copia, una barriera per passo e una copia di tutti gli `N` elementi nell'ultimo passo.
- **Il merge resta il collo di bottiglia strutturale.** Nella versione precedente, con `P = 4` valeva circa un
  terzo del tempo: l'ultimo passo fonde tutti gli `N` elementi su **un solo Worker**, mentre gli altri
  attendono (legge di Amdahl). È un vincolo della traccia, che prescrive di dimezzare i Worker attivi a ogni
  passo; vedi §8 per come superarlo.
- Il comparatore di `qsort` con due `if` è risultato **~4 % più veloce** della forma senza salti
  `(x > y) - (x < y)`: entrambe sono immuni all'overflow, ma con dati casuali il compilatore ottimizza
  meglio la prima.
- Oltre i core fisici il guadagno è **marginale**: con `N = 40 M` il minimo di 3 esecuzioni è 1,95 s con `P = 4`, 1,86 s con `P = 8` e 1,86 s con `P = 16` (≈ −5 %, probabilmente per partizioni più piccole e un migliore bilanciamento).

> Le misure dipendono dalla macchina e dal carico: vanno ripetute sul proprio hardware prima di trarre
> conclusioni. Una sola run per configurazione ha un rumore di qualche punto percentuale.

---

## 7. Limiti noti e aree ancora aperte

| | Aspetto | Dettaglio |
|---|---|---|
| 🟠 | **Ultimo merge sequenziale** | Vedi §6. Si risolve con un merge parallelo (co-ranking), che però esce dal testo della traccia. |
| 🟡 | **`memcpy` finale nel `main`** | Con passi dispari costa una copia di `N` interi su un solo thread. Evitabile restituendo il puntatore al buffer che contiene il risultato. |
| 🟡 | **Barriera POSIX** | La traccia la ammette ("ad esempio `pthread_barrier_wait`"), ma parla di sincronizzazione con mutex e variabili di condizione: una barriera scritta a mano (contatore + generazione) userebbe lo stesso schema della coda. |
| 🟡 | **Un solo tipo di dati** | Solo array di `int` generati con `rand()`; per `N` grande `rand() % range` ha un piccolo bias (modulo) e `RAND_MAX` limita il range. |
| 🟡 | **Test su un solo tipo di input** | Tutti gli input sono casuali: mancano array già ordinati, in ordine inverso, tutti uguali o con molti duplicati. |
| 🟡 | **Fallimento di `pthread_create`** | Dopo l'errore il programma termina con `exit`; va bene per l'esercizio, ma non fa `join` né pulizia dei thread già partiti. |
| 🟡 | **`N` massimo `2³¹−1`** | Imposto da `MAX_N`; il codice usa `long`, quindi il limite è solo di memoria (due buffer di `N` interi). |

---

## 8. Sviluppi futuri

In ordine crescente di impegno, per trasformare l'esercizio in un piccolo progetto di approfondimento.

1. **Distribuzioni di input** (`-d random|sorted|reverse|dups|equal`) per testare `qsort` e i merge sui casi
   limite, e un test che le provi tutte.
2. **Barriera scritta a mano** con mutex e variabile di condizione (contatore + numero di generazione, per
   renderla riutilizzabile), confrontata per prestazioni con `pthread_barrier_t`.
3. **Merge parallelo con co-ranking (*merge path*)**: ogni merge viene spezzato in sottointervalli con una
   ricerca binaria sul punto di taglio, così che anche l'ultimo passo usi tutti i Worker. Attacca il
   collo di bottiglia di §6. *Ipotesi non ancora misurata in questo repository*; è fuori dalla traccia,
   quindi andrebbe affiancata alla versione fedele e confrontata.
4. **Sorting diverso da `qsort`** — `qsort` chiama una funzione per ogni confronto; un introsort con confronto
   inline o un *radix sort* sugli `int` riduce molto il costo della fase di sorting, che oggi domina.
5. **Più partizioni dei Worker per default** (es. `4·P`) e misura dell'effetto sul bilanciamento con input
   non uniformi o core eterogenei.
6. **Benchmark riproducibile** — script che varia `P`, `Q` e `N`, ripete le misure, calcola media e
   deviazione e produce una tabella o un grafico (come già fatto nell'Esercitazione 5), con confronto con
   `qsort` e `sort -n` sequenziali.
7. **Affinità e topologia** — `pthread_setaffinity_np`, attenzione a NUMA e hyper-threading.
8. **CI** — GitHub Actions che esegue `make check` con gcc e clang; i test restituiscono già l'exit code giusto.
9. **Confronto con altri modelli** — la stessa pipeline con OpenMP (`#pragma omp task`) o un *thread pool*
   persistente, per vedere cosa guadagna e cosa perde il codice rispetto alla versione POSIX esplicita.

---

## 9. Come è stato verificato

| Verifica | Esito |
|---|---|
| `make` con `-Wall -Wextra -std=gnu11` | nessun warning |
| `make test` — `P ∈ {1…8, 16}` × `N ∈ {1, 2, 3, 7, 8, 30, 31, 1000, 10007}`, partizioni `-p` ≠ `P`, argomenti non validi, 40 ripetizioni con semi diversi, stress fino a 2 000 000 elementi; ogni run confrontata con un `qsort` sequenziale (`-c`) | **142/142** |
| `make test-asan` (AddressSanitizer + UBSan) | 54/54, nessuna segnalazione |
| `make test-tsan` (ThreadSanitizer) | 54/54, **nessuna data race** |
| **Controllo di mutazione**: bug introdotti apposta in una copia — merge che perde un elemento, `memcpy` finale mancante, resto delle partizioni ignorato, merge in ordine inverso | la suite li **rileva tutti e 4**. Un quinto cambiamento (non saltare i task vuoti) non è rilevato perché è innocuo: `qsort` su 0 elementi non fa nulla |
| `N = 40 000 000` con `P = 1, 2, 3, 4, 8, 16` (con `-c` per `P = 3` e `8`) | ordinato correttamente |

Per rilanciare tutto: `make check`.

Il quadro generale dell'intera repository è in [`../../docs/ANALISI_CODICE.md`](../../docs/ANALISI_CODICE.md).
