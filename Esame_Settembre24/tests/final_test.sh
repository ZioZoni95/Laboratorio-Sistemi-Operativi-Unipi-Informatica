#!/bin/bash

# Definire la cartella di output per i log
LOG_FILE="../Esame_Settembre24/output/log.txt"

# Cambia nella cartella principale dove si trova il Makefile
cd ../

echo "Compilazione dei file..."
make clean && make

# Esegui il test con valori di default
echo "Esecuzione del test con valori di default... (R = 5, T = 3, B = 10)" >> $LOG_FILE
./build/fabbrica_biciclette >> $LOG_FILE 2>&1
if [ $? -ne 0 ]; then
    echo "Test fallito! Riavvio..." >> $LOG_FILE
    ./build/fabbrica_biciclette >> $LOG_FILE 2>&1
    if [ $? -ne 0 ]; then
        echo "Riavvio fallito. Uscita..." >> $LOG_FILE
        exit 1  # Esci se il riavvio fallisce
    fi
fi

# Test con aumento della capacità
echo "Test con aumento della capacità con R = 10 T = 5 B = 20" >> $LOG_FILE
./build/fabbrica_biciclette R=10 T=6 B=20 >> $LOG_FILE 2>&1
if [ $? -ne 0 ]; then
    echo "Test fallito durante aumento capacità! Riavvio..." >> $LOG_FILE
    ./build/fabbrica_biciclette R=10 T=6 B=20 >> $LOG_FILE 2>&1
    if [ $? -ne 0 ]; then
        echo "Riavvio fallito. Uscita..." >> $LOG_FILE
        exit 1
    fi
fi

# Test con sincronizzazione simultanea
echo "Test con sincronizzazione simultanea..." >> $LOG_FILE
for i in {1..5}
do
    ./build/fabbrica_biciclette >> $LOG_FILE 2>&1 &
done
wait
if [ $? -ne 0 ]; then
    echo "Test di sincronizzazione fallito! Riavvio..." >> $LOG_FILE
    for i in {1..5}
    do
        ./build/fabbrica_biciclette >> $LOG_FILE 2>&1 &
    done
    wait
    if [ $? -ne 0 ]; then
        echo "Riavvio fallito. Uscita..." >> $LOG_FILE
        exit 1
    fi
fi

# Test di concorrenza con un numero elevato di thread...
echo "Test di concorrenza con un numero elevato di thread..." >> $LOG_FILE
for i in {1..50}
do
    ./build/fabbrica_biciclette >> $LOG_FILE 2>&1 &
done
wait
if [ $? -ne 0 ]; then
    echo "Test di concorrenza fallito! Riavvio..." >> $LOG_FILE
    for i in {1..50}
    do
        ./build/fabbrica_biciclette >> $LOG_FILE 2>&1 &
    done
    wait
    if [ $? -ne 0 ]; then
        echo "Riavvio fallito. Uscita..." >> $LOG_FILE
        exit 1
    fi
fi


# Aggiungere qui altri test se necessario...

echo "Test eseguiti" >> $LOG_FILE
