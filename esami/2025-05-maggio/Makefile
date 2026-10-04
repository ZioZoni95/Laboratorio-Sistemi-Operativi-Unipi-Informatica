CC      ?= gcc
CFLAGS  ?= -O2 -g
override CFLAGS += -std=gnu11 -Wall -Wextra -pthread -Iinclude
LDFLAGS += -pthread

SRCDIR = src
OBJDIR = obj
SOURCES = $(wildcard $(SRCDIR)/*.c)
HEADERS = $(wildcard include/*.h)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

EXECUTABLE = parallel_sort

# Opzioni per i controlli dinamici (eseguibili separati, sorgenti ricompilati)
SAN_FLAGS = -std=gnu11 -Wall -Wextra -pthread -Iinclude -g -O1

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) Makefile
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ThreadSanitizer (data race) e AddressSanitizer + UBSan (memoria, comportamenti indefiniti)
$(EXECUTABLE)_tsan: $(SOURCES) $(HEADERS)
	$(CC) $(SAN_FLAGS) -fsanitize=thread $(SOURCES) -o $@

$(EXECUTABLE)_asan: $(SOURCES) $(HEADERS)
	$(CC) $(SAN_FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(SOURCES) -o $@

# Batteria di test: BIN indica l'eseguibile da provare (default: quello normale).
test: $(EXECUTABLE)
	BIN=./$(EXECUTABLE) bash ./test.sh

test-tsan: $(EXECUTABLE)_tsan
	BIN=./$(EXECUTABLE)_tsan QUICK=1 bash ./test.sh

test-asan: $(EXECUTABLE)_asan
	BIN=./$(EXECUTABLE)_asan QUICK=1 bash ./test.sh

# Tutti i controlli: normale + sanitizer
check: test test-asan test-tsan

clean:
	rm -rf $(OBJDIR) log $(EXECUTABLE) $(EXECUTABLE)_tsan $(EXECUTABLE)_asan

.PHONY: all test test-tsan test-asan check clean
