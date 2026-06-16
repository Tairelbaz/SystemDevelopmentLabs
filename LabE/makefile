CC = gcc
CFLAGS = -m32 -g -Wall -D_FILE_OFFSET_BITS=64

myELF: myELF.c
	$(CC) $(CFLAGS) -o myELF myELF.c

clean:
	rm -f myELF out.ro

.PHONY: clean
