#!/bin/bash

# ==========================================================
#  Script di Test per Valutazione Progetto Parallel Sort
#  Focalizzato su W=1 e W=2 (come potenze di 2 valide)
# ==========================================================
# Questo script esegue una serie di test sul programma parallel_sort
# per verificarne la correttezza.
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
    # Nota: La traccia originale del progetto chiede che il programma gestisca
    # P non potenza di 2, ma qui ci concentriamo su P=1 e P=2 che sono potenze di 2.
    # Il check per P non potenza di 2 è qui mantenuto per completezza nel caso
    # tu voglia reintrodurre tali test, ma non dovrebbe scattare per W=1 o W=2.
    if echo "$description" | grep -q "non potenza di 2"; then
        # Questo blocco ora si aspetta un fallimento se il tuo main.c è stato modificato
        # per uscire con errore per P non potenza di 2.
        # Se il tuo main.c emette solo un avviso e continua, questo test fallirà.
        if [ $EXIT_CODE -ne 0 ]; then # Assumiamo che il programma esca con errore
            status="[OK] (Programma terminato con errore come atteso per P non potenza di 2)"
        elif echo "$OUTPUT" | grep -qi "avviso:.*non è una potenza di 2"; then
             status="[OK] (Avviso per P non potenza di 2 emesso, ma il programma ha continuato - potrebbe essere il comportamento desiderato)"
        else
             status="[ERRORE] (Il programma NON ha fallito né emesso un avviso standard per P non potenza di 2)"
        fi
    # Caso 2: Test che dovrebbero avere successo (W=1 o W=2)
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
echo ">>> FASE 2: Esecuzione Batteria di Test (Focus W=1, W=2) <<<"
rm -f test_*_output.txt # Pulisce output precedenti

# Test Correttezza Base con W=1 (Sequenziale)
run_test "T01_W1" "$PROGRAM -n 50 -w 1"  "Correttezza: Caso Sequenziale (W=1, N=50)"
run_test "T02_W1" "$PROGRAM -n 1 -w 1"   "Correttezza: Caso Sequenziale (W=1, N=1)"
run_test "T03_W1" "$PROGRAM -n 10 -w 1"  "Correttezza: Caso Sequenziale (W=1, N=10)"

# Test Correttezza Base con W=2
run_test "T04_W2" "$PROGRAM -n 50 -w 2"  "Correttezza: W=2, N=50"
run_test "T05_W2" "$PROGRAM -n 2 -w 2"   "Correttezza: W=2, N=2 (N=W)"
run_test "T06_W2" "$PROGRAM -n 10 -w 2"  "Correttezza: W=2, N=10"
run_test "T07_W2" "$PROGRAM -n 1 -w 2"   "Correttezza: W=2, N=1 (N < W)" # Particolarmente interessante

# Test Casi Limite con W=1 e W=2
run_test "T08_LIMIT" "$PROGRAM -n 8 -w 1"   "Caso Limite: N=8, W=1"
run_test "T09_LIMIT" "$PROGRAM -n 8 -w 2"   "Caso Limite: N=8, W=2"
run_test "T10_LIMIT" "$PROGRAM -n 7 -w 1"   "Caso Limite: N=7 (dispari), W=1"
run_test "T11_LIMIT" "$PROGRAM -n 7 -w 2"   "Caso Limite: N=7 (dispari), W=2 (N > W)"
run_test "T12_LIMIT" "$PROGRAM -n 1 -w 2"   "Caso Limite Ripetuto: N=1, W=2 (N < W)" # Verifica consistenza

# Test con N più grande (per W=1 e W=2)
# Usiamo 'time' per dare un'idea, ma l'output sarà incluso nel file
run_test "T13_SCALE" "time $PROGRAM -n 10000 -w 1" "Scalabilità Indicativa: N Grande, W=1"
run_test "T14_SCALE" "time $PROGRAM -n 10000 -w 2" "Scalabilità Indicativa: N Grande, W=2"

# Test opzionale: P non potenza di 2 (se vuoi verificare come il tuo programma attuale lo gestisce)
# Se il tuo programma deve fallire, modifica la condizione nel `run_test`
# run_test "T15_NON_POW2" "$PROGRAM -n 50 -w 3"  "Input Errato: W=3 (non potenza di 2)"

echo ">>> BATTERIA DI TEST COMPLETATA <<<"
echo "Verificare lo stato [OK]/[ERRORE] per ciascun test."
echo "I file 'test_*.txt' contengono l'output dettagliato di ogni esecuzione."
echo "=========================================================="

exit 0