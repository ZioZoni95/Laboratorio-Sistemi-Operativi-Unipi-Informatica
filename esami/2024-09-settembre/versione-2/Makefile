# Nome dell'eseguibile
TARGET = prod_bici

# Cartelle
SRC_DIR = .
INC_DIR = include

# File sorgenti
SRCS = $(SRC_DIR)/main.c $(INC_DIR)/queue.c

# Compilatore e flag
CC = gcc
CFLAGS = -Wall -std=c11 -I$(INC_DIR) -lpthread

# Regola principale
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

# Pulizia dei file compilati
clean:
	rm -f $(TARGET)

# Regola per test (se vuoi aggiungerli dopo)
test: all
	@echo "Eseguiamo dei test..."

.PHONY: all clean test
