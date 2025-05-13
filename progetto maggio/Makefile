# Makefile per Parallel Sort

# --- Variabili Configurabili ---

# Compilatore C
CC = gcc

# Flags per il compilatore C:
# -Wall -Wextra : Abilita molti warning utili
# -std=c11      : Usa lo standard C11
# -O2           : Livello di ottimizzazione (puoi usare -O0 per debug senza ottimizzazioni)
# -pthread      : Abilita il supporto Pthreads (necessario per compilazione e linking)
# -Iinclude     : Dice al compilatore dove cercare i file .h (nella cartella 'include')
# -D_POSIX_C_SOURCE=200809L : Definisce la macro per abilitare le feature POSIX (come le barriere)
CFLAGS =  -g -Wall -Wextra -std=c11 -O2 -pthread -Iinclude -D_POSIX_C_SOURCE=200809L

# Flags per il linker:
# -pthread : Necessario anche al linker per collegare la libreria Pthreads
# -lm      : Collega la libreria matematica (per funzioni come log2, se usate)
LDFLAGS = -pthread -lm

# Nome dell'eseguibile finale
TARGET = parallel_sort

# --- Definizione Cartelle ---

SRCDIR = src
INCDIR = include
OBJDIR = obj

# --- Individuazione Automatica File ---

# Trova tutti i file .c nella cartella SRCDIR
SRCS = $(wildcard $(SRCDIR)/*.c)

# Genera i nomi dei file oggetto (.o) corrispondenti, posizionandoli in OBJDIR
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# --- Regole Principali ---

# Target di default: crea l'eseguibile. Dipende dall'eseguibile $(TARGET).
all: $(TARGET)

# Regola per creare l'eseguibile finale $(TARGET):
# Dipende da tutti i file oggetto ($(OBJS)).
# Collega gli oggetti usando il linker (CC) con i flag LDFLAGS.
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)
	@echo "$(TARGET) creato con successo."

# --- Regole di Compilazione ---

# Regola pattern per compilare un file .c da SRCDIR in un file .o in OBJDIR:
# Dipende dal corrispondente file .c e dalla directory OBJDIR (vedi | $(OBJDIR)).
# Compila usando il compilatore (CC) con i flag CFLAGS.
# '$<' è il nome del primo prerequisito (il file .c).
# '$@' è il nome del target (il file .o).
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# --- Regole Utilità ---

# Regola per creare la directory OBJDIR se non esiste:
# Viene eseguita una sola volta grazie alla dipendenza "order-only" (| $(OBJDIR))
# nella regola di compilazione.
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# Target per pulire i file generati (oggetti ed eseguibile)
clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJDIR) $(TARGET)
	@echo "Clean complete."

# Target per eseguire con parametri di esempio (compila prima se necessario)
# Ridotto N per evitare output di debug eccessivo se DEBUG è attivo
run: all
	./$(TARGET) -n 10000 -p 4

# Target per eseguire con parametri personalizzati (compila prima se necessario)
# Uso: make run_custom N=<numero_elementi> P=<numero_thread>
run_custom: all
	./$(TARGET) -n $(N) -p $(P)

# --- Dichiarazione Target Fittizi ---
# Dichiara che questi target non rappresentano file reali.
.PHONY: all clean run run_custom