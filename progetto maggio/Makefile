CC = gcc
CFLAGS = -g -Wall -Wextra -std=c99 -O2 -pthread -fsanitize=thread -Iinclude -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread -lm -fsanitize=thread
TARGET = parallel_sort
SRCDIR = src
INCDIR = include
OBJDIR = obj
LOG_DIR = log
SRCS = $(wildcard $(SRCDIR)/*.c)
HDRS = $(wildcard $(INCDIR)/*.h)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)
	@echo "$(TARGET) creato con successo."

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HDRS) | $(OBJDIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	@mkdir -p $(OBJDIR)

test: $(TARGET)
	@echo "Esecuzione dei test con test.sh..."
	@chmod +x test.sh
	@./test.sh
	@echo "Test completati. Controllare l'output e i file in $(LOG_DIR)/"

clean:
	@echo "Cleaning build files and logs..."
	@rm -rf $(OBJDIR) $(TARGET)
	@rm -f $(LOG_DIR)/test_output_*.txt
	@rmdir $(LOG_DIR) 2>/dev/null || true
	@echo "Clean complete."

run: all
	./$(TARGET) -n 10000 -w 4

run_custom: all
	./$(TARGET) -n $(N) -w $(P)

.PHONY: all clean test run run_custom