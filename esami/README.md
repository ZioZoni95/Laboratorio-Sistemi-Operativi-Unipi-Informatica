# Mini-progetti d'esame

Una cartella per sessione, in ordine cronologico. Ogni progetto è un programma C multi-thread da
consegnare con `Makefile` e test scritti dallo studente.

| Sessione | Traccia | Cartella | Stato |
|---|---|---|---|
| Luglio 2023 | Struttura dati condivisa da 4 thread (T1/T2 scrittori, T3/T4 lettori) con la politica di accesso descritta in traccia, **solo semafori** | [`2023-07-luglio`](2023-07-luglio) | Compila. `make` + `test.sh`. Contiene anche gli appunti di soluzione (`guida_esame_luglio2023_semafori.pdf`) |
| Settembre 2024 | Fabbrica di biciclette: produttori di ruote e telai, un assemblatore, porta-ruote/porta-telai a capacità limitata | [`2024-09-settembre/versione-1`](2024-09-settembre/versione-1) | Compila con `make`, test in `tests/` (con ASan/UBSan). Contiene `src/main copy.c`, file residuo che non compila |
| | | [`2024-09-settembre/versione-2`](2024-09-settembre/versione-2) | Seconda versione, con la coda in `include/queue.c`; `make` + `test.sh` (un warning per `usleep` non dichiarata) |
| Marzo 2025 | Somma parallela di un vettore con un thread **Master** e *P* **Worker**, coda **limitata** di capacità *C*, gruppi di *k* coppie — [`Appello-Marzo.pdf`](2025-03-marzo/Appello-Marzo.pdf) | [`2025-03-marzo/new_ProgettoMarzo25`](2025-03-marzo/new_ProgettoMarzo25) | Versione funzionante: `make` → `esame_MasterWorker`, somma corretta nelle prove fatte |
| | | [`2025-03-marzo/progetto_marzo_gem`](2025-03-marzo/progetto_marzo_gem) | Contiene **due** implementazioni mescolate (`*.c` e `*2.c`): il `Makefile` le compila insieme e il link fallisce (vedi analisi) |
| | | [`2025-03-marzo/Progetto_marzo`](2025-03-marzo/Progetto_marzo) | Implementazione senza Makefile; con varianti di debug (`main_deb.c`, `threads_debug.c`) e `pseudo.c` (pseudocodice della consegna, non compilabile) |
| Maggio 2025 | Ordinamento parallelo di un vettore con *P* Worker, coda concorrente di task `(start,end)`, `qsort` per partizione e merge in log₂P passi con barriere — [`Appello-Maggio.pdf`](2025-05-maggio/Appello-Maggio.pdf) | [`2025-05-maggio`](2025-05-maggio/README.md) | Completo e migliorato: qualunque `P ≥ 1`, opzione `-p`, 142 test (`make check` con ASan/UBSan e TSan puliti), pseudocodice della traccia |

> Per il progetto di marzo (non consegnato in tempo ma completato) le diverse cartelle sono implementazioni
> alternative dello stesso esercizio; la più pulita è `new_ProgettoMarzo25`.

Compilazione tipica: `cd <cartella> && make`. Per i problemi noti di ogni progetto vedi
[`../docs/ANALISI_CODICE.md`](../docs/ANALISI_CODICE.md).
