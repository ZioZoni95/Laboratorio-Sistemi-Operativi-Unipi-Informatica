# Emulatore PlayStation 1 in C

Questo è un emulatore per PlayStation 1 scritto in C. L'obiettivo è emulare l'hardware della PS1, inclusi la CPU MIPS R3000A, la memoria, la GPU, la SPU e altri componenti.

## Funzionalità
- Emulazione della CPU
- Gestione della memoria
- Rendering GPU (basato su framebuffer)
- Riproduzione audio
- Gestione degli input

## Guida all'uso
1. Installa `gcc` o `clang`.
2. Posiziona il file BIOS della PS1 (`scph1001.bin`) nella cartella `tests/`.
3. Compila l'emulatore usando il comando `make`.
4. Esegui l'emulatore con ROM di test.

## Licenza
Solo per scopi educativi. Non utilizzare BIOS o ROM protetti da copyright.
