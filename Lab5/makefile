CC = gcc
CFLAGS = -m32 -Wall -g

all: loader

loader: loader.c
	$(CC) $(CFLAGS) -o loader loader.c

.PHONY: all clean
clean:
	rm -f loader
