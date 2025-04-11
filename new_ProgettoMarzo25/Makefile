CC = gcc
CFLAGS = -Wall -std=c11 -Iinclude -pthread
SRC = src/queue.c src/master.c src/worker.c src/main.c
OBJ = $(SRC:.c=.o)
TARGET = esame_MasterWorker

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)