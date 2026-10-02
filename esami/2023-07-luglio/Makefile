# Makefile per la compilazione del progetto
CC = gcc
CFLAGS = -Wall -pthread -I.  # Aggiunto -I. per includere la directory corrente

TARGET = main
OBJS = sync.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lpthread

sync.o: sync.c
	$(CC) $(CFLAGS) -c sync.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(TARGET) $(OBJS)
