#!/bin/bash
# ==========================================================
#  Batteria di test di parallel_sort
#
#  Uso:   ./test.sh                 (dopo `make`, oppure direttamente `make test`)
#         BIN=./parallel_sort_tsan QUICK=1 ./test.sh
#
#  Variabili:
#    BIN    eseguibile da provare (default ./parallel_sort)
#    QUICK  se impostata, riduce la matrice dei test (per ASan/TSan, che rallentano molto)
#
#  Exit code: 0 se TUTTI i test passano, 1 altrimenti.
# ==========================================================

BIN="${BIN:-./parallel_sort}"
LOG_DIR="log"
TIMEOUT=120          # secondi: un blocco (deadlock) fa fallire il test invece di bloccare lo script
PASSED=0
FAILED=0

if [ ! -x "$BIN" ]; then
    echo "ERRORE: eseguibile '$BIN' non trovato (esegui prima 'make')." >&2
    exit 1
fi
mkdir -p "$LOG_DIR"
rm -f "$LOG_DIR"/fail_*.txt

report_fail() {   # $1=descrizione  $2=file di output  $3=dettaglio
    FAILED=$((FAILED + 1))
    local f="$LOG_DIR/fail_$FAILED.txt"
    { echo "# $1"; echo "# $3"; cat "$2"; } > "$f"
    echo "  [FALLITO] $1 -> $3 (dettagli: $f)"
}

# Test positivo: il programma deve terminare con exit 0 e dichiarare l'array ordinato.
# Il parametro -c confronta il risultato anche con un qsort sequenziale (verifica esatta).
expect_ok() {     # $1=N  $2=P  [altre opzioni...]
    local n=$1 p=$2; shift 2
    local out; out=$(mktemp)
    timeout "$TIMEOUT" "$BIN" -n "$n" -w "$p" -c "$@" > "$out" 2>&1
    local code=$?
    if [ $code -eq 124 ]; then
        report_fail "N=$n P=$p $*" "$out" "timeout dopo ${TIMEOUT}s (blocco?)"
    elif [ $code -ne 0 ]; then
        report_fail "N=$n P=$p $*" "$out" "exit code $code"
    elif ! grep -q "L'array è ordinato correttamente" "$out"; then
        report_fail "N=$n P=$p $*" "$out" "manca l'esito positivo della verifica"
    else
        PASSED=$((PASSED + 1))
    fi
    rm -f "$out"
}

# Test negativo: argomenti non validi; il programma deve fallire con un messaggio d'errore.
expect_error() {  # $1=descrizione  poi gli argomenti del programma
    local desc=$1; shift
    local out; out=$(mktemp)
    timeout "$TIMEOUT" "$BIN" "$@" > "$out" 2>&1
    local code=$?
    if [ $code -eq 124 ]; then
        report_fail "$desc" "$out" "timeout (il programma doveva rifiutare gli argomenti)"
    elif [ $code -eq 0 ]; then
        report_fail "$desc" "$out" "exit 0: gli argomenti non validi dovevano essere rifiutati"
    elif ! grep -qi "errore\|uso:" "$out"; then
        report_fail "$desc" "$out" "nessun messaggio d'errore"
    else
        PASSED=$((PASSED + 1))
    fi
    rm -f "$out"
}

echo "== Correttezza: P x N (P non solo potenza di 2; N < P, N = P, N non multiplo di P) =="
if [ -n "$QUICK" ]; then
    WORKERS="1 2 3 4 8"; SIZES="1 3 8 31 1000"
else
    WORKERS="1 2 3 4 5 6 7 8 16"; SIZES="1 2 3 7 8 30 31 1000 10007"
fi
for p in $WORKERS; do
    for n in $SIZES; do
        expect_ok "$n" "$p" -s $((n * 31 + p))
    done
done

echo "== Numero di partizioni diverso da P (-p) =="
for cfg in "100 4 1" "100 4 2" "100 4 3" "100 4 7" "100 4 16" "7 2 20" "1000 3 50"; do
    set -- $cfg
    expect_ok "$1" "$2" -p "$3" -s 7
done

echo "== Argomenti non validi =="
expect_error "senza argomenti"
expect_error "manca -w"            -n 10
expect_error "manca -n"            -w 4
expect_error "-n 0"                -n 0 -w 4
expect_error "-n negativo"         -n -5 -w 4
expect_error "-w 0"                -n 10 -w 0
expect_error "-n non numerico"     -n abc -w 4
expect_error "-n con suffisso"     -n 12abc -w 4
expect_error "-w non numerico"     -n 10 -w xyz
expect_error "-p 0"                -n 10 -w 2 -p 0
expect_error "opzione sconosciuta" -n 10 -w 2 -z
expect_error "argomento extra"     -n 10 -w 2 extra

echo "== Ripetizioni con semi diversi (per far emergere eventuali race) =="
REPS=40; [ -n "$QUICK" ] && REPS=10
for i in $(seq 1 $REPS); do
    expect_ok $((50 + i * 13)) 8 -s "$i"
done

if [ -z "$QUICK" ]; then
    echo "== Stress =="
    START=$(date +%s.%N)
    expect_ok 2000000 4 -s 42
    expect_ok 1000001 6 -s 43
    END=$(date +%s.%N)
    awk -v s="$START" -v e="$END" 'BEGIN { printf "  (stress completato in %.1f s, incluso il confronto con qsort)\n", e - s }'
fi

echo "=========================================================="
echo "Eseguibile: $BIN"
echo "Test superati: $PASSED   Test falliti: $FAILED"
if [ $FAILED -gt 0 ]; then
    echo "ESITO: FALLITO (vedi $LOG_DIR/)"
    exit 1
fi
echo "ESITO: OK"
exit 0
