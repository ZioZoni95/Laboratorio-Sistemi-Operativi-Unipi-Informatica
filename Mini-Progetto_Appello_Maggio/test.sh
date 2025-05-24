#!/bin/bash

# ==========================================================
#  Script di Test
# ==========================================================
PROGRAM="./parallel_sort"
MAKEFILE="Makefile"
LOG_DIR="log"

run_test() {
    local test_id="$1"
    local command="$2"
    local description="$3"
    local output_file="$LOG_DIR/test_output_${test_id}.txt"
    local status="[ERRORE]"

    echo "----------------------------------------------------------"
    echo "[TEST $test_id] $description"
    echo "Comando Eseguito: $command"
    echo "----------------------------------------------------------"

    OUTPUT=$($command 2>&1 | tee "$output_file")
    EXIT_CODE=${PIPESTATUS[0]}

    echo ""

    if echo "$description" | grep -q "Errore Atteso"; then # Modificato per cercare "Errore Atteso"
        if [ $EXIT_CODE -ne 0 ] && echo "$OUTPUT" | grep -q "ERRORE: Il numero di worker P=.*non è una potenza di 2"; then
            status="[OK] (Programma terminato con errore come atteso)"
        else
            status="[ERRORE] (P non potenza di 2: Comportamento inatteso. EXIT_CODE=$EXIT_CODE. Output: $OUTPUT)"
        fi
    else
        if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "Verifica: L'array è ordinato correttamente."; then
            status="[OK]"
        else
            status="[ERRORE] (EXIT_CODE=$EXIT_CODE. Verifica ordinamento fallita o output anomalo)"
        fi
    fi

    echo "Esito Test $test_id: $status"
    echo "Output completo salvato in: $output_file"
    echo "=========================================================="
    echo ""
    sleep 1
}

echo ">>> FASE 0: Preparazione Directory Log <<<"
mkdir -p "$LOG_DIR"
rm -f "$LOG_DIR"/test_output_*.txt
echo "Directory log '$LOG_DIR' pronta."
echo ""

echo ">>> FASE 1: Compilazione del Progetto <<<"
if [ ! -f "$MAKEFILE" ]; then
    echo "[ERRORE] Makefile non trovato!" && exit 1
fi
make clean && make
if [ $? -ne 0 ]; then
    echo "[ERRORE] Compilazione fallita!" && exit 1
fi
if [ ! -x "$PROGRAM" ]; then
    echo "[ERRORE] Eseguibile '$PROGRAM' non trovato!" && exit 1
fi
echo "Compilazione completata con successo."
echo ""

echo ">>> FASE 2: Esecuzione Batteria di Test Ridotta <<<"

# === Test P=1 (Sequenziale) ===
run_test "P1_N1"  "$PROGRAM -n 1 -w 1"   "Correttezza: P=1, N=1"
run_test "P1_N30" "$PROGRAM -n 30 -w 1"  "Correttezza: P=1, N=30"

# === Test P=2 (Un Passo di Merge) ===
run_test "P2_N1"  "$PROGRAM -n 1 -w 2"   "Correttezza: P=2, N=1 (N < P)"
run_test "P2_N2"  "$PROGRAM -n 2 -w 2"   "Correttezza: P=2, N=2 (N = P)"
run_test "P2_N30" "$PROGRAM -n 30 -w 2"  "Correttezza: P=2, N=30 (N > P, N%P==0)"
run_test "P2_N31" "$PROGRAM -n 31 -w 2"  "Correttezza: P=2, N=31 (N > P, N%P!=0)"

# === Test P=4 (Due Passi di Merge) ===
run_test "P4_N1"  "$PROGRAM -n 1 -w 4"   "Correttezza: P=4, N=1 (N < P)"
run_test "P4_N3"  "$PROGRAM -n 3 -w 4"   "Correttezza: P=4, N=3 (N < P)"
run_test "P4_N4"  "$PROGRAM -n 4 -w 4"   "Correttezza: P=4, N=4 (N = P)"
run_test "P4_N30" "$PROGRAM -n 30 -w 4"  "Correttezza: P=4, N=30 (N > P)"



# === Test di "Stress" (opzionale, puoi commentarlo se troppo lento) ===
run_test "Stress_P4_N5k" "time $PROGRAM -n 5000 -w 4" "Stress: P=4, N=5000"

echo ">>> BATTERIA DI TEST COMPLETATA <<<"
echo "Verificare lo stato [OK]/[ERRORE] per ciascun test."
echo "I file di output dei test sono salvati nella directory: $LOG_DIR/"
echo "=========================================================="

exit 0