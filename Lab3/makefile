CC = gcc
CFLAGS = -m32 -Wall -ansi -c -nostdlib -fno-stack-protector
ASM = nasm
ASMFLAGS = -f elf32
LD = ld
LDFLAGS = -m elf_i386

task2: start.o main.o util.o
	$(LD) $(LDFLAGS) start.o main.o util.o -o task2

start.o: start.s
	$(ASM) $(ASMFLAGS) start.s -o start.o

main.o: main.c util.h
	$(CC) $(CFLAGS) main.c -o main.o

util.o: util.c util.h
	$(CC) $(CFLAGS) util.c -o util.o

.PHONY: clean
clean:
	rm -f *.o task2
