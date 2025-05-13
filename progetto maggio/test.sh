#!/bin/bash

# ==========================================================
#  Script di Test per Progetto Ordinamento Parallelo
# ==========================================================
# Questo script esegue una serie di test sul programma parallel_sort
# per verificarne la correttezza, focalizzandosi sui casi rilevanti
# come da traccia d'esame (P potenze di 2, N vs P).

PROGRAM="./parallel_sort"
MAKEFILE="Makefile"

# --- Funzione Helper per Eseguire e Valutare un Test ---
# Args: $1=ID Test, $2=Comando da eseguire, $3=Descrizione Test
run_test() {
    local test_id="$1"
    local command="$2"
    local description="$3"
    local output_file="test_output_${test_id}.txt" # Nome file output modificato per chiarezza
    local status="[ERRORE]" # Default a Errore

    echo "----------------------------------------------------------"
    echo "[TEST $test_id] $description"
    echo "Comando Eseguito: $command"
    echo "----------------------------------------------------------"

    # Esegue il comando e salva l'output su file e variabile
    # Stampa l'output anche a schermo durante l'esecuzione
    # stderr (2) viene rediretto a stdout (1) per catturare sia output normale che errori/avvisi
    OUTPUT=$($command 2>&1 | tee "$output_file")
    EXIT_CODE=${PIPESTATUS[0]} # Cattura l'exit code del programma, non di tee

    echo "" # Riga vuota per separare output da valutazione

    # Valutazione Automatica
    if echo "$description" | grep -q "P non potenza di 2"; then
        # Per P non potenza di 2, ci aspettiamo un avviso e che il programma
        # termini correttamente (EXIT_CODE 0) o con un codice di errore specifico
        # se si decidesse di non supportarli affatto.
        # Il main.c attuale stampa un avviso e continua.
        if echo "$OUTPUT" | grep -qi "avviso:.*non è una potenza di 2"; then
            if [ $EXIT_CODE -eq 0 ]; then
                if echo "$OUTPUT" | grep -q "Verifica: L'array è ordinato correttamente."; then
                    status="[OK] (Avviso P non potenza di 2 emesso. Ordinamento OK)"
                else
                    status="[ATTENZIONE] (Avviso P non potenza di 2 emesso. Ordinamento FALLITO o non verificato, EXIT_CODE 0)"
                fi
            else
                status="[ATTENZIONE] (Avviso P non potenza di 2 emesso, ma terminato con EXIT_CODE $EXIT_CODE)"
            fi
        else
             status="[ERRORE] (P non potenza di 2: Nessun avviso standard emesso e/o EXIT_CODE $EXIT_CODE inatteso)"
        fi
    # Caso standard: Test che dovrebbero avere successo (P potenza di 2)
    else
        if [ $EXIT_CODE -eq 0 ] && echo "$OUTPUT" | grep -q "Verifica: L'array è ordinato correttamente."; then
            status="[OK]"
        elif [ $EXIT_CODE -ne 0 ]; then
             status="[ERRORE] (Terminato con codice di errore $EXIT_CODE)"
        else
             status="[ERRORE] (Terminato con EXIT_CODE 0, ma verifica dell'ordinamento fallita o non trovata)"
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
rm -f test_output_*.txt # Pulisce output precedenti

# === Test con P=1 (Caso Sequenziale) ===
echo "--- Test con P=1 (Caso Sequenziale) ---"
run_test "P1_N1"  "$PROGRAM -n 1 -w 1"   "Correttezza: P=1, N=1"
run_test "P1_N10" "$PROGRAM -n 10 -w 1"  "Correttezza: P=1, N=10"
run_test "P1_N50" "$PROGRAM -n 50 -w 1"  "Correttezza: P=1, N=50"

# === Test con P=2 (Un Passo di Merge) ===
echo "--- Test con P=2 (Un Passo di Merge) ---"
run_test "P2_N1"  "$PROGRAM -n 1 -w 2"   "Correttezza: P=2, N=1 (N < P)"
run_test "P2_N2"  "$PROGRAM -n 2 -w 2"   "Correttezza: P=2, N=2 (N = P)"
run_test "P2_N7"  "$PROGRAM -n 7 -w 2"   "Correttezza: P=2, N=7 (N > P, N dispari)"
run_test "P2_N10" "$PROGRAM -n 10 -w 2"  "Correttezza: P=2, N=10 (N > P, N pari)"
run_test "P2_N50" "$PROGRAM -n 50 -w 2"  "Correttezza: P=2, N=50"

# === Test con P=4 (Due Passi di Merge) ===
echo "--- Test con P=4 (Due Passi di Merge) ---"
run_test "P4_N1"  "$PROGRAM -n 1 -w 4"   "Correttezza: P=4, N=1 (N < P)"
run_test "P4_N3"  "$PROGRAM -n 3 -w 4"   "Correttezza: P=4, N=3 (N < P)"
run_test "P4_N4"  "$PROGRAM -n 4 -w 4"   "Correttezza: P=4, N=4 (N = P)"
run_test "P4_N10" "$PROGRAM -n 10 -w 4"  "Correttezza: P=4, N=10 (N > P, partizioni N%P != 0)"
run_test "P4_N12" "$PROGRAM -n 12 -w 4"  "Correttezza: P=4, N=12 (N > P, partizioni N%P == 0)"
run_test "P4_N50" "$PROGRAM -n 50 -w 4"  "Correttezza: P=4, N=50"

# === Test con P=8 (Tre Passi di Merge) ===
echo "--- Test con P=8 (Tre Passi di Merge) ---"
run_test "P8_N1"  "$PROGRAM -n 1 -w 8"   "Correttezza: P=8, N=1 (N < P)"
run_test "P8_N7"  "$PROGRAM -n 7 -w 8"   "Correttezza: P=8, N=7 (N < P)"
run_test "P8_N8"  "$PROGRAM -n 8 -w 8"   "Correttezza: P=8, N=8 (N = P)"
run_test "P8_N15" "$PROGRAM -n 15 -w 8"  "Correttezza: P=8, N=15 (N > P, N dispari)"
run_test "P8_N16" "$PROGRAM -n 16 -w 8"  "Correttezza: P=8, N=16 (N > P, N pari)"
run_test "P8_N50" "$PROGRAM -n 50 -w 8"  "Correttezza: P=8, N=50"

# === Test con P non potenza di 2 (Verifica Avviso e Stabilità) ===
# Ci si aspetta un avviso e che il programma non crashi. L'ordinamento potrebbe essere corretto o meno.
echo "--- Test con P non potenza di 2 (Verifica Avviso e Stabilità) ---"
run_test "P3_N50_NonPow2" "$PROGRAM -n 50 -w 3" "Avviso/Stabilità: P=3 (non potenza di 2), N=50"
run_test "P6_N50_NonPow2" "$PROGRAM -n 50 -w 6" "Avviso/Stabilità: P=6 (non potenza di 2), N=50"

# === Test di "Stress" / Scalabilità Indicativa ===
# L'output di 'time' sarà incluso nel file di log del test.
echo "--- Test di Stress / Scalabilità Indicativa ---"
run_test "Stress_P1_N10k"  "time $PROGRAM -n 10000 -w 1" "Stress: P=1, N=10000"
run_test "Stress_P4_N10k"  "time $PROGRAM -n 10000 -w 4" "Stress: P=4, N=10000"
run_test "Stress_P8_N10k"  "time $PROGRAM -n 10000 -w 8" "Stress: P=8, N=10000"


echo ">>> BATTERIA DI TEST COMPLETATA <<<"
echo "Verificare lo stato [OK]/[ERRORE]/[ATTENZIONE] per ciascun test."
echo "I file 'test_output_*.txt' contengono l'output dettagliato di ogni esecuzione."
echo "=========================================================="

exit 0