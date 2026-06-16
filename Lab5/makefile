CC     = gcc
CFLAGS = -m32 -Wall -g
LD     = ld
# linking_script supplies OUTPUT_FORMAT(elf32-i386) + ENTRY(_start), so -m
# elf_i386 is unnecessary. Objects must precede -L/-l; -lc resolves through
# /usr/lib32/libc.so to /lib32/libc.so.6, loaded by the dynamic linker below.
LDFLAGS = -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

all: loader

loader: loader.o startup.o start.o
	$(LD) -o loader loader.o startup.o start.o $(LDFLAGS)

loader.o: loader.c
	$(CC) $(CFLAGS) -c loader.c -o loader.o

start.o: start.s
	nasm -f elf32 start.s -o start.o

startup.o: startup.s
	nasm -f elf32 startup.s -o startup.o

.PHONY: all clean
clean:
	rm -f loader loader.o start.o startup.o
