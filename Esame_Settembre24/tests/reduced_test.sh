#!/bin/bash

# Definire la cartella di output per i log
LOG_FILE="../Esame_Settembre24/output/log.txt"

# Cambia nella cartella principale dove si trova il Makefile
cd ../

echo "Compilazione dei file..." | tee -a $LOG_FILE
make clean && make | tee -a $LOG_FILE

# Esegui il test con valori di default
echo "Esecuzione del test con valori di default... (R = 5, T = 3, B = 10)" | tee -a $LOG_FILE
./build/fabbrica_biciclette | tee -a $LOG_FILE
if [ $? -ne 0 ]; then
    echo "Test fallito! Riavvio..." | tee -a $LOG_FILE
    ./build/fabbrica_biciclette | tee -a $LOG_FILE
    if [ $? -ne 0 ]; then
        echo "Riavvio fallito. Uscita..." | tee -a $LOG_FILE
        exit 1  # Esci se il riavvio fallisce
    fi
fi

# Test con aumento della capacità (solo una volta)
echo "Test con aumento della capacità con R = 10 T = 5 B = 20" | tee -a $LOG_FILE
./build/fabbrica_biciclette R=10 T=5 B=20 | tee -a $LOG_FILE
if [ $? -ne 0 ]; then
    echo "Test fallito durante aumento capacità! Riavvio..." | tee -a $LOG_FILE
    ./build/fabbrica_biciclette R=10 T=5 B=20 | tee -a $LOG_FILE
    if [ $? -ne 0 ]; then
        echo "Riavvio fallito. Uscita..." | tee -a $LOG_FILE
        exit 1
    fi
fi

# Test con sincronizzazione simultanea (solo 2 esecuzioni)
echo "Test con sincronizzazione simultanea..." | tee -a $LOG_FILE
for i in {1..2}
do
    ./build/fabbrica_biciclette | tee -a $LOG_FILE &
done
wait
if [ $? -ne 0 ]; then
    echo "Test di sincronizzazione fallito! Riavvio..." | tee -a $LOG_FILE
    for i in {1..2}
    do
        ./build/fabbrica_biciclette | tee -a $LOG_FILE &
    done
    wait
    if [ $? -ne 0 ]; then
        echo "Riavvio fallito. Uscita..." | tee -a $LOG_FILE
        exit 1
    fi
fi

# Test di concorrenza con un numero ridotto di thread (5 esecuzioni)
echo "Test di concorrenza con un numero ridotto di thread..." | tee -a $LOG_FILE
for i in {1..5}
do
    ./build/fabbrica_biciclette | tee -a $LOG_FILE &
done
wait
if [ $? -ne 0 ]; then
    echo "Test di concorrenza fallito! Riavvio..." | tee -a $LOG_FILE
    for i in {1..5}
    do
        ./build/fabbrica_biciclette | tee -a $LOG_FILE &
    done
    wait
    if [ $? -ne 0 ]; then
        echo "Riavvio fallito. Uscita..." | tee -a $LOG_FILE
        exit 1
    fi
fi

# Aggiungere qui altri test se necessario...

echo "Test eseguiti" | tee -a $LOG_FILE
