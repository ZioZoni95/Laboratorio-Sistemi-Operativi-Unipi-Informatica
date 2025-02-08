CC = gcc
CFLAGS = -Wall -pthread -g -fsanitize=address,undefined

SRC_DIR = src
BUILD_DIR = build
OUTPUT_DIR = output

# Definisci i file sorgenti e gli oggetti
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/queue.c $(SRC_DIR)/worker.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
EXEC = $(BUILD_DIR)/fabbrica_biciclette

# Crea la cartella build se non esiste
$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(OUTPUT_DIR))

# Regola per compilare il programma
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)
	@echo "Compilazione completata"

# Regola per compilare i file oggetto nella cartella build
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulisci i file di compilazione
clean:
	@echo "Pulizia dei file..."
	rm -f $(OBJS) $(EXEC)
	@echo "Pulizia completata"

# Esegui i test
run_tests:
	@echo "Esecuzione di tutti i test..."
	./tests/test.sh > $(OUTPUT_DIR)/log.txt 2>&1

