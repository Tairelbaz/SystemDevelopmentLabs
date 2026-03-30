# All Targets
all: main my_echo
# Executable "main" depends on main.o, numbers.o and add.o
main: main.o numbers.o add.o 
	gcc -m32 -g -Wall -o main main.o numbers.o add.o
my_echo: my_echo.o
	gcc -m32 -g -Wall -o my_echo my_echo.o
# Depends on the source and header files
main.o: main.c
	gcc -g -Wall -m32 -c -o main.o main.c
add.o: add.s
	nasm -g -f elf -w+all -o add.o add.s
numbers.o: numbers.c
	gcc -g -Wall -m32 -c -o numbers.o numbers.c
my_echo.o: my_echo.c
	gcc -g -Wall -m32 -c -o my_echo.o my_echo.c
# tell make that "clean" is not a file name!
.PHONY: clean
# Clean the build directory
clean:
	rm -f *.o main
