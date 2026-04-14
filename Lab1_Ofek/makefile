# All Targets
all: count-words addresses base menu
# Executables
count-words: count-words.o
	gcc -m32 -g -Wall -o count-words count-words.o
addresses: addresses.o
	gcc -m32 -g -Wall -o addresses addresses.o
base: base.o
	gcc -m32 -g -Wall -o base base.o
menu: menu_map.o
	gcc -m32 -g -Wall -o menu menu_map.o
# Depends on the source files
count-words.o: count-words.c
	gcc -g -Wall -m32 -c -o count-words.o count-words.c
addresses.o: addresses.c
	gcc -g -Wall -m32 -c -o addresses.o addresses.c
base.o: base.c
	gcc -g -Wall -m32 -c -o base.o base.c
menu_map.o: menu_map.c
	gcc -g -Wall -m32 -c -o menu_map.o menu_map.c
# tell make that "clean" is not a file name!
.PHONY: clean
# Clean the build directory
clean:
	rm -f *.o count-words addresses base menu
