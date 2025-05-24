#!/bin/bash

EXEC=./prod_bici
MAKE_CMD="make -s"
TEST_COUNT=0
PASS_COUNT=0

# Funzione di pulizia e compilazione
echo "Pulizia e compilazione del progetto..."
make clean
$MAKE_CMD || { echo "❌ Compilazione fallita"; exit 1; }

run_test() {
  TEST_COUNT=$((TEST_COUNT + 1))  DESC=$1
  CMD=$2
  EXPECTED=$3

  echo "------------------------------------"
  echo "🧪 Test #$TEST_COUNT: $DESC"
  echo "🔧 Eseguo: $CMD"

  # Esegui il comando e cattura l'output
  OUTPUT=$($CMD)
  echo "$OUTPUT"

  # Verifica se l'output contiene la stringa attesa
  MATCH=$(echo "$OUTPUT" | grep "Totale biciclette assemblate: $EXPECTED")
  if [ -n "$MATCH" ]; then
    echo "✅ Test superato"
    PASS_COUNT=$((PASS_COUNT + 1))

  else
    echo "❌ Test fallito (atteso: $EXPECTED)"
  fi
  echo "------------------------------------"
  echo
}

# TEST CASES
run_test "Valori default" "$EXEC" 10
run_test "Valori compatti (4 4 2)" "$EXEC 4 4 2" 2
run_test "Valori espliciti (R_CAPACITY=6 T_CAPACITY=6 B_MAX=3)" "$EXEC R_CAPACITY=6 T_CAPACITY=6 B_MAX=3" 3
run_test "Valori grandi (6 6 15)" "$EXEC 6 6 15" 15

# Riepilogo
echo "=============================="
echo "✅ $PASS_COUNT / $TEST_COUNT test superati"
echo "=============================="

# Se ci sono test falliti, termina con codice di errore
exit $((TEST_COUNT - PASS_COUNT))
