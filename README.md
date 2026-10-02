# Laboratorio di Sistemi Operativi — Unipi, Informatica

![images](https://github.com/user-attachments/assets/4d955caf-ca28-4aff-aa7c-264d71d1d5d3)

Esercizi del laboratorio di **Sistemi Operativi** (Università di Pisa, Corso di Laurea in Informatica,
**vecchio ordinamento**), più i mini-progetti d'esame svolti nelle sessioni 2023-2025.
Tutto il codice è in **C** (POSIX, `pthread`) e si compila con `gcc` su Linux.

## Struttura della repository

```
.
├── esercitazioni/          Esercitazioni di laboratorio 1-8 (+ lettori/scrittori)
├── esami/                  Mini-progetti d'esame, uno per sessione
│   ├── 2023-07-luglio/        Accesso a struttura condivisa da 4 thread, solo semafori
│   ├── 2024-09-settembre/     Fabbrica di biciclette (due versioni)
│   ├── 2025-03-marzo/         Somma parallela Master/Worker con coda limitata (tre implementazioni)
│   └── 2025-05-maggio/        Ordinamento parallelo con coda concorrente e merge a barriere  ← progetto principale
├── esempi-lezione/         Esempi mostrati a lezione (fork/exec, thread, shell minimale)
├── librerie/               Codice di supporto riusabile (tabella hash `icl_hash`, FIFO illimitata)
├── soluzioni-didawiki/     Archivi `.tgz` con tracce/soluzioni scaricate dalla didawiki del corso
└── docs/
    └── ANALISI_CODICE.md   Analisi del codice: problemi trovati e aree di miglioramento
```

Ogni cartella ha un proprio README con l'elenco dei contenuti:

| Sezione | Contenuto |
|---|---|
| [`esercitazioni/`](esercitazioni/README.md) | Un esercizio per file, con argomento e comando di compilazione |
| [`esami/`](esami/README.md) | Tracce, versioni dei progetti e stato di ciascuna |
| [`esami/2025-05-maggio/`](esami/2025-05-maggio/README.md) | Il progetto più curato: Makefile, suite di test, uso e design |
| [`docs/ANALISI_CODICE.md`](docs/ANALISI_CODICE.md) | Bug, rischi e miglioramenti, ordinati per priorità |

## Requisiti

- Linux (o WSL) con `gcc` ≥ 9 e `make`
- Facoltativi: `valgrind`, e il supporto a `-fsanitize=thread/address` di gcc per i controlli dinamici

## Compilare ed eseguire

Il progetto con Makefile e test più completo è quello di maggio:

```bash
cd esami/2025-05-maggio
make            # compila → ./parallel_sort
make test       # esegue la batteria di test (test.sh)
./parallel_sort -n 100000 -w 4
```

Per gli altri esercizi vedi il comando di compilazione indicato nei README di sezione. In generale:

```bash
gcc -Wall -Wextra -pthread file.c -o file
```

## Note

- Gli **eseguibili e i file oggetto non sono versionati** (vedi `.gitignore`): vanno rigenerati con `make` o `gcc`.
- Il codice è stato scritto durante il corso e a scopo di studio: alcune cartelle contengono varianti
  e tentativi successivi dello stesso esercizio (es. `versione-1`/`versione-2`). I problemi noti sono
  elencati in [`docs/ANALISI_CODICE.md`](docs/ANALISI_CODICE.md).
- Le tracce d'esame (PDF) appartengono al corso e sono incluse solo come riferimento.
