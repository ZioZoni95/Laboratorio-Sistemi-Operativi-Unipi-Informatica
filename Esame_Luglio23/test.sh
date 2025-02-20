#### Script di test test.sh
#!/bin/bash

echo "Compilazione del programma..."
make clean && make

if [ $? -ne 0 ]; then
    echo "Errore durante la compilazione."
    exit 1
fi

echo "Esecuzione del programma..."
./main

exit 0