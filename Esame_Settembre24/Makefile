CC = gcc
CFLAGS = -Wall -pthread

SRC = src/main.c src/worker.c src/queue.c
OBJ = $(SRC:.c=.o)
TARGET = fabbrica_biciclette

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) $(OBJ)
