#!/bin/bash

# ==========================================================
#  Script di Test per Valutazione Progetto Parallel Sort
# ==========================================================
# Questo script esegue una serie di test sul programma parallel_sort
# per verificarne la correttezza in base ai requisiti dell'esame.
# Ogni test viene etichettato e il suo risultato (OK/ERRORE)
# viene determinato automaticamente analizzando l'output del programma.

PROGRAM="./parallel_sort"
MAKEFILE="Makefile"

# --- Funzione Helper per Eseguire e Valutare un Test ---
# Args: $1=ID Test, $2=Comando da eseguire, $3=Descrizione Test
run_test() {
    local test_id="$1"
    local command="$2"
    local description="$3"
    local output_file="test_${test_id}_output.txt"
    local status="[ERRORE]" # Default a Errore

    echo "----------------------------------------------------------"
    echo "[TEST $test_id] $description"
    echo "Comando: $command"
    echo "----------------------------------------------------------"

    # Esegue il comando e salva l'output su file e variabile
    # Stampa l'output anche a schermo durante l'esecuzione
    OUTPUT=$($command 2>&1 | tee "$output_file")
    EXIT_CODE=$?

    echo "" # Riga vuota per separare output da valutazione

    # Valutazione Automatica
    # Caso 1: W non è potenza di 2 (Errore Atteso)
    if echo "$description" | grep -q "non potenza di 2"; then
        if [ $EXIT_CODE -ne 0 ] && echo "$OUTPUT" | grep -q "deve essere una potenza di 2"; then
            status="[OK] (Errore atteso gestito correttamente)"
        elif [ $EXIT_CODE -eq 0 ]; then
             status="[ERRORE] (Il programma NON ha fallito come atteso per W non potenza di 2)"
        else
             status="[ERRORE] (Fallito, ma messaggio di errore per W non potenza di 2 non trovato)"
        fi
    # Caso 2: Test che dovrebbero avere successo
    else
        if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "Verifica: L'array è ordinato correttamente."; then
            status="[OK]"
        elif [ $EXIT_CODE -ne 0 ]; then
             status="[ERRORE] (Terminato con codice di errore $EXIT_CODE)"
        else
             status="[ERRORE] (Terminato correttamente, ma verifica dell'ordinamento fallita o non trovata)"
        fi
    fi

    echo "Esito Test $test_id: $status"
    echo "Output completo salvato in: $output_file"
    echo "=========================================================="
    echo ""
    sleep 1 # Pausa per leggibilità
}

# --- Fase 1: Compilazione ---
echo ">>> FASE 1: Compilazione del Progetto <<<"
if [ ! -f "$MAKEFILE" ]; then
    echo "[ERRORE] Makefile non trovato! Impossibile compilare."
    exit 1
fi
make clean && make
if [ $? -ne 0 ]; then
    echo "[ERRORE] Compilazione fallita! Lo script di test non può continuare."
    exit 1
fi
if [ ! -x "$PROGRAM" ]; then
    echo "[ERRORE] Eseguibile '$PROGRAM' non trovato dopo la compilazione!"
    exit 1
fi
echo "Compilazione completata con successo."
echo ""

# --- Fase 2: Esecuzione Test ---
echo ">>> FASE 2: Esecuzione Batteria di Test <<<"
rm -f test_*_output.txt # Pulisce output precedenti

# Test Correttezza Base (N piccolo, W potenze di 2)
run_test "T01" "$PROGRAM -n 50 -w 1"  "Correttezza: Caso Sequenziale (W=1)"
run_test "T02" "$PROGRAM -n 50 -w 2"  "Correttezza: W=2"
run_test "T03" "$PROGRAM -n 50 -w 4"  "Correttezza: W=4"
run_test "T04" "$PROGRAM -n 50 -w 8"  "Correttezza: W=8"

# Test Casi Limite (N vs W)
run_test "T05" "$PROGRAM -n 8 -w 8"   "Caso Limite: N = W"
run_test "T06" "$PROGRAM -n 7 -w 8"   "Caso Limite: N < W"
run_test "T07" "$PROGRAM -n 16 -w 16" "Caso Limite: N = W (più grande)"
run_test "T08" "$PROGRAM -n 15 -w 16" "Caso Limite: N < W (più grande)"

# Test Gestione Input Errati (W non potenza di 2)
run_test "T09" "$PROGRAM -n 50 -w 3"  "Input Errato: W=3 (non potenza di 2)"
run_test "T10" "$PROGRAM -n 50 -w 6"  "Input Errato: W=6 (non potenza di 2)"
run_test "T11" "$PROGRAM -n 50 -w 7"  "Input Errato: W=7 (non potenza di 2)"

# Test Scalabilità Indicativa (N grande) - Nota: la correttezza è ancora verificata
# Usiamo 'time' per dare un'idea, ma l'output sarà incluso nel file
run_test "T12" "time $PROGRAM -n 50000 -w 1" "Scalabilità: N Grande, W=1"
run_test "T13" "time $PROGRAM -n 50000 -w 4" "Scalabilità: N Grande, W=4"
run_test "T14" "time $PROGRAM -n 50000 -w 8" "Scalabilità: N Grande, W=8"

echo ">>> BATTERIA DI TEST COMPLETATA <<<"
echo "Verificare lo stato [OK]/[ERRORE] per ciascun test."
echo "I file 'test_*.txt' contengono l'output dettagliato di ogni esecuzione."
echo "=========================================================="

exit 0