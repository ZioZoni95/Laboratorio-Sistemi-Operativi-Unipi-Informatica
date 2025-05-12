#!/bin/bash

# Script per eseguire una batteria di test concisa per parallel_sort

echo "====================================================="
echo " ESECUZIONE BATTERIA DI TEST CONCISA PARALLEL SORT "
echo "====================================================="
echo ""

# --- Preparazione ---
# 1. Assicurarsi che l'inizializzazione in src/main.c sia impostata su CASUALE:
#    array[i] = rand() % (n * 10);
# 2. Compilare il progetto:
echo "--> Compilazione del progetto (assicurati che l'init sia CASUALE)..."
make clean > /dev/null && make
if [ $? -ne 0 ]; then
    echo "ERRORE: Compilazione fallita!"
    exit 1
fi
echo "Compilazione completata."
echo ""

# Funzione helper per eseguire un test e mostrare delimitatori
run_test() {
    TEST_ID=$1
    CMD=$2
    DESC=$3
    echo "--- TEST $TEST_ID: $DESC ---"
    echo "Comando: $CMD"
    echo "Output:"
    # Esegue il comando e cattura l'output e lo stato di uscita
    OUTPUT=$($CMD)
    EXIT_CODE=$?
    echo "$OUTPUT"
    # Verifica base dello stato di uscita e dell'output di verifica
    if [ $EXIT_CODE -ne 0 ]; then
        echo "** ERRORE: Il programma ha terminato con codice $EXIT_CODE **"
    elif ! echo "$OUTPUT" | grep -q "Verifica: L'array è ordinato correttamente."; then
        echo "** ERRORE: Output di verifica non trovato o indica errore! **"
    else
        echo "** OK: Terminato correttamente e ordinamento verificato. **"
    fi
    echo "---------------------------------"
    echo ""
    sleep 0.5 # Piccola pausa
}

# --- Test Base e Variazione P (N=30, Dati Casuali) ---
echo "*** SEZIONE 1: Correttezza Base e Variazione P (N=30) ***"
run_test "CT1" "./parallel_sort -n 30 -p 1" "Sequenziale (P=1)"
run_test "CT2" "./parallel_sort -n 30 -p 2" "Parallelo (P=2)"
run_test "CT3" "./parallel_sort -n 30 -p 4" "Parallelo (P=4)"
run_test "CT4" "./parallel_sort -n 30 -p 8" "Parallelo (P=8)"

# --- Test Casi Limite N/P (Dati Casuali) ---
echo "*** SEZIONE 2: Casi Limite N/P ***"
run_test "CT5" "./parallel_sort -n 8 -p 8" "N = P"
run_test "CT6" "./parallel_sort -n 6 -p 8" "N < P"

# --- Test Scalabilità Indicativa (N=100k, Dati Casuali) ---
echo "*** SEZIONE 3: Scalabilità Indicativa (N=100000) ***"
run_test "CT9" "time ./parallel_sort -n 100000 -p 1" "Baseline N Grande (P=1)"
run_test "CT10" "time ./parallel_sort -n 100000 -p 4" "Parallelo N Grande (P=4)"


# --- Guida per Test Dati Specifici ---
echo "*** SEZIONE 4: Tipi di Dati Specifici (N=50, P=4) ***"
echo "I seguenti test richiedono una modifica manuale di 'src/main.c',"
echo "seguita da ricompilazione ('make') ed esecuzione manuale:"
echo ""
echo "  Test CT6 (Già Ordinato):"
echo "    1. Modifica src/main.c -> array[i] = i;"
echo "    2. Esegui: make"
echo "    3. Esegui: ./parallel_sort -n 50 -p 4"
echo ""
echo "  Test CT7 (Ordine Inverso):"
echo "    1. Modifica src/main.c -> array[i] = 49 - i; // (N=50 -> N-1-i = 49-i)"
echo "    2. Esegui: make"
echo "    3. Esegui: ./parallel_sort -n 50 -p 4"
echo ""
echo "  Test CT8 (Duplicati):"
echo "    1. Modifica src/main.c -> array[i] = rand() % 5;"
echo "    2. Esegui: make"
echo "    3. Esegui: ./parallel_sort -n 50 -p 4"
echo ""
echo "(Ricorda di ripristinare l'inizializzazione casuale in src/main.c dopo questi test!)"
echo ""

echo "====================================================="
echo " BATTERIA DI TEST COMPLETATA "
echo "====================================================="
echo "Verificare l'output di ogni test per i messaggi '** OK ... **'."
echo "Per CT9/CT10, osservare i tempi 'real' riportati da 'time'."