all: task0 my_echo

task0: main.o numbers.o add.o
	gcc -m32 -g -Wall -o task0 main.o numbers.o add.o

main.o: main.c
	gcc -m32 -g -Wall -c -o main.o main.c

numbers.o: numbers.c
	gcc -m32 -g -Wall -c -o numbers.o numbers.c

add.o: add.s
	nasm -g -f elf -w+all -o add.o add.s

my_echo: my_echo.c
	gcc -m32 -g -Wall -o my_echo my_echo.c

encoder: encoder.c
	gcc -m32 -g -Wall -o encoder encoder.c

clean:
	rm -f *.o task0 my_echo encoder
