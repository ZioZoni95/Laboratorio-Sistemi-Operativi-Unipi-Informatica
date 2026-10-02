CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude -g
LDFLAGS = -pthread

SRCDIR = src
OBJDIR = obj
INCDIR = include

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

EXECUTABLE = parallel_sort

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Eseguibile $(EXECUTABLE) creato con successo."

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(wildcard $(INCDIR)/*.h) Makefile
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compilato $< -> $@"

clean:
	@echo "Pulizia dei file generati..."
	rm -rf $(OBJDIR)
	rm -f $(EXECUTABLE)
	@echo "Pulizia completata."

test: all
	@echo "Esecuzione della batteria di test..."
	@if [ -f ./test.sh ]; then \
		bash ./test.sh; \
	else \
		echo "ERRORE: Script di test 'test.sh' non trovato nella directory corrente."; \
		exit 1; \
	fi

.PHONY: all clean test