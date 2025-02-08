#!/bin/sh

# Funzione per eseguire un test e verificare il risultato
run_test() {
    description=$1
    command=$2
    expected_output=$3

    echo "Test: $description"
    echo "Esecuzione comando: $command"
    output=$($command 2>&1)  # Cattura anche gli errori

    # Confronta l'output con quello atteso, ignorando spazi aggiuntivi e nuove linee
    if echo "$output" | grep -q "^$expected_output$"; then
        echo "Test PASSED"
    else
        echo "Test FAILED"
        echo "Output ottenuto:"
        echo "$output"
    fi
    echo "-----------------------------------------"
}

# Test 1: Directory vuota
mkdir -p testdir1
run_test "Directory vuota" \
    "./mylsdir testdir1" \
    "Directory: testdir1"

# Test 2: Directory con file
mkdir -p testdir2
touch testdir2/file1.txt testdir2/file2.txt
chmod 755 testdir2/file1.txt
chmod 644 testdir2/file2.txt
run_test "Directory con file" \
    "./mylsdir testdir2" \
    "Directory: testdir2
file1.txt           0 rwxr-xr-x
file2.txt           0 rw-r--r--"

# Test 3: Directory con sottodirectory
mkdir -p testdir3/subdir1
touch testdir3/subdir1/subfile1.txt
chmod 755 testdir3/subdir1/subfile1.txt
run_test "Directory con sottodirectory" \
    "./mylsdir testdir3" \
    "Directory: testdir3
Directory: testdir3/subdir1
subfile1.txt        0 rwxr-xr-x"

# Test 4: Directory con file nascosti
mkdir -p testdir4
touch testdir4/.hiddenfile
chmod 644 testdir4/.hiddenfile
run_test "Directory con file nascosti" \
    "./mylsdir testdir4" \
    "Directory: testdir4"

# Test 5: Directory con errori di accesso
mkdir -p testdir5
chmod 000 testdir5
run_test "Directory con errori di accesso" \
    "./mylsdir testdir5" \
    "Impossibile aprire la directory testdir5"

# Test 6: Directory con link simbolici
mkdir -p testdir6
touch testdir6/realfile.txt
ln -s testdir6/realfile.txt testdir6/symlink
chmod 755 testdir6/symlink
run_test "Directory con link simbolici" \
    "./mylsdir testdir6" \
    "Directory: testdir6
realfile.txt        0 rw-r--r--
symlink             0 rwxr-xr-x"

# Pulisci le directory di test
rm -rf testdir1 testdir2 testdir3 testdir4 testdir5 testdir6

echo "Tutti i test sono stati completati."
