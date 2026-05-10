# Makefile Guide

## Compiling C and Assembly Projects in Linux

### Introduction

In order to make our source code executable, we need first to compile all the C files and then link the compilation products into an executable file. If we have external libraries we need to link those as well. We can do this manually through the console, but it can be time-consuming for larger projects. Alternatively, we can use a makefile, which contains all the data and commands needed to transform source code files into an executable program.

Assume our project contains: `HelloWorld.c`, `HelloWorld.h`, and `Run.c`. In this course you are required to use the flags shown here.

### Compiling a C Project Manually

To compile the hello world project:

1. Open a terminal.
2. Go to your project's root, e.g. `cd ~/splab/labA`
3. Clean: `rm -f *.o hello`
4. Compile: `gcc -g -m32 -Wall -c -o hello.o HelloWorld.c`
5. Compile: `gcc -g -m32 -Wall -c -o Run.o Run.c`
6. Link: `gcc -g -m32 -Wall -o hello hello.o Run.o`

**Notice:** we do not compile a header file.

### gcc Parameters

| Flag | Description |
| --- | --- |
| `-o fileName` | Place output in file fileName; creates by default a file called `a.out`. |
| `-c` | Compile or assemble the source files, but do not link. The compiler output is an object file corresponding to each source file. |
| `-Ldir` | Add directory dir to the list of directories to be searched for `-l`. |
| `-llib` | Use the library named lib when linking. |
| `-O` | Optimize. Use this to yield optimized code (e.g. code that runs faster). |
| `-g` | Add debug information. |
| `-W` | What warning level you would like to receive. |
| `-m32` | Specifies the output file format to be 32-bit. |

### Compiling a C Project with Assembly Files

Assuming we have `HelloWorld.c`, `HelloWorld.h`, and `start.s`:

1. Open a terminal.
2. Go to your project's root, e.g. `cd ~/splab/labA`
3. Clean: `rm -f *.o hello`
4. Compile: `gcc -m32 -g -Wall -c -o hello.o HelloWorld.c`
5. Compile: `nasm -g -f elf -w+all -o start.o start.s`
6. Link: `gcc -m32 -g -Wall -o hello hello.o start.o`

The only difference is that assembly language files are compiled by the assembler (`nasm`), rather than the C compiler `gcc`. Linking object files from the assembler works the same way.

### nasm Parameters

| Flag | Description |
| --- | --- |
| `-o fileName` | Place output in file fileName; creates by default a file called `[original filename].o`. |
| `-f format` | Specifies the output file format. Use `-hf` to see valid output formats. |
| `-g` | Add debug information. |
| `-w+all` | Warning level to receive. |

### Understanding Makefiles

Makefiles (and the `make` program which uses them) specify dependencies between abstract targets and what needs to be done to achieve each target. A Makefile is a text file containing a description of targets, followed by their dependencies and the actions necessary to build them. Target names and dependencies are assumed to be files.

Consider a simple target T1 which depends on D1 and D2. The `make` utility works as follows:

1. If either D1 or D2 do not exist, build them (recursively).
2. Check if both D1 and D2 are up to date. If not, build them recursively.
3. Check the modification time of T1. If T1 is at least as new as both D1 and D2, we are done.
4. Otherwise, follow the instructions given to build T1.

When you type `make` in your shell, it looks for a file called "makefile" in the same directory and executes it. By default, `make` will only execute the first target. So make sure the first target causes a complete build.

### Makefile Format

```
# format is target-name: target dependencies
# {tab} actions
```

### C Project Makefile

```makefile
# All Targets
all: hello

# Executable "hello" depends on hello.o and Run.o
hello: hello.o Run.o
	gcc -g -m32 -Wall -o hello hello.o Run.o

# Depends on the source and header files
hello.o: HelloWorld.c HelloWorld.h
	gcc -m32 -g -Wall -c -o hello.o HelloWorld.c

Run.o: Run.c
	gcc -m32 -g -Wall -c -o Run.o Run.c

# tell make that "clean" is not a file name!
.PHONY: clean

# Clean the build directory
clean:
	rm -f *.o hello
```

### C Project with Assembly Makefile

```makefile
# All Targets
all: hello

# Executable "hello" depends on hello.o and start.o
hello: hello.o start.o
	gcc -m32 -g -Wall -o hello hello.o start.o

# Depends on the source and header files
hello.o: HelloWorld.c HelloWorld.h
	gcc -g -Wall -m32 -c -o hello.o HelloWorld.c

start.o: start.s
	nasm -g -f elf -w+all -o start.o start.s

# tell make that "clean" is not a file name!
.PHONY: clean

# Clean the build directory
clean:
	rm -f *.o hello
```

**Important:** The indentation to the left of action lines must be TAB characters, not spaces. Some text editors replace tabs with spaces — beware of this!

Makefiles are simple text files. You can create them with any text editor (notepad, kwrite, gedit, nano, vim, etc.).
