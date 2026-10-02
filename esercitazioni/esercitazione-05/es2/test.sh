#!/bin/bash

# Imposta una directory di test
TEST_DIR="test_myfind_dir"
TARGET_FILE="target.txt"

# Creazione della struttura di test
mkdir -p "$TEST_DIR/subdir1" "$TEST_DIR/subdir2"
echo "Test file 1" > "$TEST_DIR/$TARGET_FILE"
echo "Test file 2" > "$TEST_DIR/subdir1/$TARGET_FILE"
echo "Test file 3" > "$TEST_DIR/subdir2/other.txt"



# Verifica se la compilazione è riuscita
if [ $? -ne 0 ]; then
    echo "Errore nella compilazione di myfind.c"
    exit 1
fi

# Esegui il programma e confronta l'output con 'find'
echo "Eseguo myfind:"
./myfind "$TEST_DIR" "$TARGET_FILE"

echo "Eseguo find per confronto:"
find "$TEST_DIR" -type f -name "$TARGET_FILE"

# Pulizia: rimuove la directory di test
rm -rf "$TEST_DIR"
rm -f myfind
