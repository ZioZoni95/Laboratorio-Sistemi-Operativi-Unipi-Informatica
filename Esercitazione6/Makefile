# Compiler
CC = gcc
CFLAGS = -g -Wall -fsanitize=address

# Directory degli esercizi
DIRS = es1 es2 es3 es4 es5

# File sorgenti ed eseguibili
SRC_ES1 = es1/dummyshell.c es1/dummyshell_optimized.c
SRC_ES2 = es2/background_exec.c
SRC_ES3 = es3/zombie.c
SRC_ES4 = es4/family.c
SRC_ES5 = es5/fibo.c

# Eseguibili
BIN_ES1 = es1/dummyshell es1/dummyshell_optimized
BIN_ES2 = es2/background_exec
BIN_ES3 = es3/zombie
BIN_ES4 = es4/family
BIN_ES5 = es5/fibo

# Tutti gli eseguibili
BINARIES = $(BIN_ES1) $(BIN_ES2) $(BIN_ES3) $(BIN_ES4) $(BIN_ES5)

# Regola principale
all: $(BINARIES)

# Compilazione dei singoli eseguibili
es1/dummyshell: es1/dummyshell.c es1/utils.h
	$(CC) $(CFLAGS) -o $@ $<

es1/dummyshell_optimized: es1/dummyshell_optimized.c es1/utils.h
	$(CC) $(CFLAGS) -o $@ $<

es2/background_exec: es2/background_exec.c es2/utils.h
	$(CC) $(CFLAGS) -o $@ $<

es3/zombie: es3/zombie.c es3/utils.h
	$(CC) $(CFLAGS) -o $@ $<

es4/family: es4/family.c es4/utils.h
	$(CC) $(CFLAGS) -o $@ $<

es5/fibo: es5/fibo.c
	$(CC) $(CFLAGS) -o $@ $<

# Pulizia degli eseguibili
clean:
	rm -f $(BINARIES)
