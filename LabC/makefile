CC = gcc
CFLAGS = -Wall -g -m32

all: myshell looper mypipeline

myshell: myshell.o LineParser.o
	$(CC) $(CFLAGS) -o myshell myshell.o LineParser.o

myshell.o: myshell.c LineParser.h
	$(CC) $(CFLAGS) -c myshell.c

LineParser.o: LineParser.c LineParser.h
	$(CC) $(CFLAGS) -c LineParser.c

looper: Looper.c
	$(CC) $(CFLAGS) -o looper Looper.c

mypipeline: mypipeline.c
	$(CC) $(CFLAGS) -o mypipeline mypipeline.c

clean:
	rm -f *.o myshell looper mypipe
