Spiegazione dei punti principali di queue.c:
Gestione della sincronizzazione:
Tutte le funzioni che modificano o accedono ai dati della coda (come push, pop, top e length) utilizzano un mutex per garantire la mutua esclusione. La variabile di condizione viene impiegata per mettere in attesa i thread in pop quando la coda è vuota, e per risvegliarli quando un nuovo elemento viene inserito.

Nodo Dummy:
L'uso di un nodo dummy semplifica le operazioni di inserimento e rimozione, evitando controlli particolari per il primo elemento.

Error handling:
Le funzioni controllano se i puntatori in input sono NULL e impostano errno opportunamente.