CC = gcc
CFLAGS = -m32 -Wall -g
TASK4FLAGS = -m32 -Wall -g -fno-pie -no-pie -fno-stack-protector

all: hexeditplus task4

hexeditplus: task1.c
	$(CC) $(CFLAGS) -o hexeditplus task1.c

task4: task4.c
	$(CC) $(TASK4FLAGS) -o task4 task4.c

.PHONY: clean
clean:
	rm -f hexeditplus task4
