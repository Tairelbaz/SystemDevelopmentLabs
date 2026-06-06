ASM = nasm
ASMFLAGS = -f elf32
CC = gcc
LDFLAGS = -m32 -no-pie

all: multi

multi: multi.o
	$(CC) $(LDFLAGS) multi.o -o multi

multi.o: multi.s
	$(ASM) $(ASMFLAGS) multi.s -o multi.o

.PHONY: all clean
clean:
	rm -f multi.o multi
