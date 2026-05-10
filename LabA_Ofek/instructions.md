# Introduction to C Programming in a Unix (Linux 32-bit) Environment

## Home-Lab A

**This lab assignment is to be done SOLO!**

### Assignment Goals

- C primer
- Parsing command-line arguments
- Understanding character encoding (ASCII)
- Implementing a debug mode for your program
- Introduction to standard streams (stdin, stdout, stderr)
- Simple stream IO library functions

## Preparation – Part 0: Maintaining a Project Using make

You should perform this part before starting to implement the hand-in assignment. It is basic knowledge that is not checked, but needed to complete the assignment.

For this part, 3 files are provided: `add.s`, `main.c`, `numbers.c`. The first file is assembly language code, and the other 2 are C source code.

1. Log in to Linux.
2. Decide on an ASCII text editor of your choice (vi, emacs, kate, pico, nano, femto, or whatever). It is your responsibility to know how to operate the text editor well enough for all tasks in all labs.
3. Using your chosen text editor, write a makefile for the given files (as explained in the introduction to the GNU Make Manual; see the Reading Material for Lab A). The Makefile should provide targets for compiling the program and cleaning up the working directory.
4. Compile the project by executing `make` in the console.
5. Read all of Lab A reading material, and make sure you understand it.
6. Read the `puts(3)` and `printf(3)` manuals. What is the difference between the functions? To read the manuals type `man` followed by the function name in a console.

**Important:** To protect your files from being viewed or copied by other people, employ the Linux permission system by running: `chmod 700 -R ~`

To make sure you have sufficient space, run: `du -a | sort -n`

### Unix Control Keys

- **Control+D (^D):** Causes the Unix terminal driver to signal the EOF condition to the process running in the terminal foreground.
- **Control+C (^C):** Sends the SIGINT signal to the process running in the terminal foreground. This will usually terminate the process.
- **Control+Z (^Z):** Sends the SIGTSTP signal to the process running in the terminal foreground. This will suspend the process (the process will still live in background). **Do not use Control+Z for terminating processes!**

### Writing a Simple Program: my_echo

Re-read and understand the arguments of `main(argc, argv)`. Recall that `argc` is the number of arguments, and `argv` is an array of pointers to null-terminated strings. Write a simple echo program named `my_echo` that prints out the text given in the command line (all whitespace printed as a single space).

Example:

```
#> my_echo aa b c
aa b c
```

## The Actual Assignment

In this assignment you will write a simple encoder/decoder program for the famous Vigenere cipher. The program has three functionalities:

1. Parsing the command-line arguments and printing debug messages.
2. The actual encoder/decoder.
3. Redirecting the input and output according to the command-line arguments.

It is highly recommended that you implement each step in the above order and test it thoroughly before proceeding to the next one.

### Part 1: Command-Line Arguments, Debugging, and Input Echoing

Introduce a debug mode into your program. The minimum implementation prints out important information to stderr when in debug mode.

Loop over command-line arguments: if in debug mode, print each argument on a separate line to stderr. Debug mode is a variable initialized to "on" by default. A command-line argument `-D` turns debug mode off. A command-line argument `+Dpassword` turns debug mode on, where password is compared against a globally defined string such as:

```c
unsigned char password[] = "my_password1";
```

The effect of a debug flag change occurs starting with the next command-line argument. Use `fprintf()` for printing to stderr.

After handling command-line arguments, the program should read each input character `c` from stdin using `fgetc()`, pass it to a function called `encode(c)` that currently returns it unchanged, then print it using `fputc()` to stdout, until detecting EOF (using `feof()`). Then close the output stream and exit normally.

We recommend using global variables `infile` and `outfile` as arguments to `fgetc()` and `fputc()`, initialized to `stdin` and `stdout` respectively. This will simplify Part 3.

### Part 2: The Encoder/Decoder

Use command-line parsing to detect an encoding string and modify the behaviour of `encode()`.

Without an encoding string, every input character is sent to output unchanged.

The encryption key structure is `+V{key}` for addition and `-V{key}` for subtraction (Vigenere encoding). The key is a sequence of upper-case letters used cyclically.

For an encoding character `char`, the shift value is: `shift = (char - 'A')`. Thus 'B' adds 1, 'C' adds 2, etc.

Encoding is applied only to upper-case and lower-case letters (a–z, A–Z) with wrap-around (Z+1 = A, a-1 = z). Non-letter characters are output unchanged and do not advance the key position.

#### Examples

In this example, A,B,C,D,E are encoded by adding 1,2,3,4,5 respectively. Z+1 wraps to become A (shifted by 1 = B). Non-letter characters pass through without advancing the key:

```
#> encoder +VBCDEF
ABCDEZ
BDFHJA
12#<
12#<
^D
```

```
#> encoder -VEDCB
GDuqp202
CAspl202
^D
```

**Implementation note:** Make the encoding string a global variable with a default value of `"A"` (shift by 0) for idempotent encoding when no key is specified.

### Part 3: Input and/or Output to Specific Files

Add the option for reading input from a specified file: if `-ifname` is present, read from file `fname` instead of stdin. If `-ofname` is present, write output to file `fname` instead of stdout.

While scanning command-line arguments, check for `-i` and `-o`, open input/output files using `fopen()`, and use the returned file descriptor for `infile` and/or `outfile`. If `fopen()` fails, print an error to stderr and exit.

Your program should support encoding keys, input file, output file, and debug flag settings in any combination or order. At most one of each will be given.

### Helpful Information and Hints

- `stdin` and `stdout` are `FILE*` constants usable with `fgetc` and `fputc`.
- Make sure you know how to recognize end of file (EOF).
- Control+D in an empty input line causes `fgetc()` to return EOF; your program should then terminate normally.
- Refer to the ASCII table for character encoding information.
- Character strings in C are null-terminated arrays of `unsigned char`. Prefer comparing individual characters over using string library functions.

### Mandatory Requirements

- Read and process input character by character; no need to store characters beyond the one currently being processed.
- You cannot make any assumption about line lengths.
- Check whether a character is lowercase or uppercase using a single "if" statement with two conditions.
- You are not allowed to use any library function or macros to recognize whether a character is a letter and its case.
- Points will be deducted for using string library functions (`strlen`, `strcpy`, etc.) except `strncmp` and `strcmp`, which are allowed but not recommended.
- Read program parameters as in Part 0's `main.c`: set default values first, then scan `argv` to update them.
- Program arguments may arrive in arbitrary order.
- Output to stdout (or `ofname`) must contain only the (possibly encoded) characters from the input — any deviation will cause failing automated tests.

## Submission

Instructions:

1. Create a zip file with the relevant files only.
2. Upload the zip file to the submission system.
3. Download the zip and extract to an empty folder.
4. Compile and test the code to make sure it still works.
5. The makefile should be set so that `make encoder` generates an executable named `encoder` in the current directory.

**Deliverables:** A zip file containing exactly: `makefile` and `encoder.c`

### Credit Points per Part

| Part | Points |
| --- | --- |
| 1 – Command-Line Arguments, Debug, Echoing | 30 |
| 2 – Encoder/Decoder | 40 |
| 3 – File I/O | 30 |
