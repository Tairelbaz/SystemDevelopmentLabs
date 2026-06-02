CC = gcc
CFLAGS = -m32 -Wall -g

all: hexeditplus

hexeditplus: task1.c
	$(CC) $(CFLAGS) -o hexeditplus task1.c

.PHONY: clean
clean:
	rm -f hexeditplus
