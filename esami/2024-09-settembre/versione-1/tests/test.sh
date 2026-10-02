#!/bin/bash

#muovo nella directory
cd ..

# Definire la cartella di output per i log
LOG_FILE="../output/log.txt"

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

echo "Test eseguiti"

