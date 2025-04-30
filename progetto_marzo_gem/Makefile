# File: Makefile (Revisionato)

# --- Variabili di Configurazione ---
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -std=c11
LDFLAGS = -pthread
INC_DIR = include
SRC_DIR = src
OBJ_DIR = obj
EXEC = sum_calculator

# --- Individuazione File ---
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

# --- Regole Principali ---

# Target di default
all: $(EXEC)

# Regola per creare l'eseguibile finale:
# Dipende dall'esistenza della directory obj E da tutti i file oggetto.
$(EXEC): $(OBJ_DIR) $(OBJECTS)
	@echo "Linking..."
	$(CC) $(OBJECTS) -o $(EXEC) $(LDFLAGS)
	@echo "Build completato: Eseguibile -> $(EXEC)"

# Regola pattern per compilare i file sorgente (.c) in file oggetto (.o):
# Un target obj/%.o dipende dal corrispondente src/%.c
# Rimuoviamo la dipendenza order-only | $(OBJ_DIR) da qui.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Regola per creare la directory obj se non esiste:
# Questa regola viene invocata perché $(EXEC) dipende da $(OBJ_DIR).
$(OBJ_DIR):
	@echo "Creating object directory: $(OBJ_DIR)"
	@mkdir -p $(OBJ_DIR)

# --- Target Ausiliari ---

# Target per pulire i file generati
clean:
	@echo "Cleaning build files..."
	rm -f $(EXEC)
	rm -rf $(OBJ_DIR)
	@echo "Clean complete."

# Dichiarazione dei target "phony"
.PHONY: all clean