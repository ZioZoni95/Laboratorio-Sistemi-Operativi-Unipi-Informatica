#!/bin/bash

# Creare la cartella di output se non esiste
mkdir -p ../output

# Definire la cartella di output per i log
LOG_FILE="../output/log.txt"

# Cambia nella cartella principale dove si trova il Makefile
cd ../

echo "Compilazione dei file..."
make clean && make

echo "Esecuzione del test con valori di default... (R = 5, T = 3, B = 10)"
./fabbrica_biciclette

echo "Test con aumento della capacità"
./fabbrica_biciclette R=10 T=5 B=20

echo "Test con sincronizzazione simultanea..."
for i in {1..5}
do
    ./fabbrica_biciclette &
done
wait

echo "Test di concorrenza con un numero elevato di thread..."
for i in {1..50}
do
    ./fabbrica_biciclette &
done
wait

echo "Test con capacità massima delle risorse..."
./fabbrica_biciclette R=1000 T=1000 B=10000



echo "Test di saturazione delle risorse..."
./fabbrica_biciclette R=10 T=5 B=1000

echo "Test di gestione risorse in esaurimento..."
./fabbrica_biciclette R=2 T=3 B=10


echo "Test con combinazioni casuali di parametri..."
./fabbrica_biciclette R=15 T=8 B=50
./fabbrica_biciclette R=3 T=4 B=20
./fabbrica_biciclette R=20 T=30 B=200



echo "Test eseguiti"
