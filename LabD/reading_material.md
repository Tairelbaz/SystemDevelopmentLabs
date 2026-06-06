# Lab D: Reading Material

Lab D is dedicated to basic programming in assembly language, with emphasis on basic instructions, addressing, and calling C functions from assembly code.

This lab assumes you have already completed Lab 3.

---

## C Functions

In this lab, you are required to process the arguments passed to the `main` function. Re-read the manual page about `main` (`main man`).

For printing, you are required to use standard-library functions:

- `printf`
- `puts`

Make sure you understand the C calling convention before using these functions from assembly.

---

## Assembly and NASM

In this course we use **NASM**, the Netwide Assembler: an assembler targeting the Intel x86 series of processors.

Read the NASM manual. Review the lecture on **Assembly Language Primer**, and read about each instruction shown in the lecture in the NASM manual for better understanding.

Make sure you understand:

- Basic arithmetic commands.
- Flow-control commands.
- Effective addressing.
- Variable declarations.
- `global` and `extern` declarations.
- The 80x86 registers from the lecture slides or from the NASM manual.

---

## Little Endian and Big Endian

Endianness is the order or sequence of bytes of a word of digital data in computer memory. Endianness is primarily expressed as:

- **Big-endian (BE)**
- **Little-endian (LE)**

Read more about the differences between these methods, and make sure you understand the data layout in each method.

---

## Linear-Feedback Shift Register

Applications of LFSRs include:

- Generating pseudo-random numbers.
- Generating pseudo-noise sequences.
- Implementing fast digital counters.
- Whitening sequences.

Both hardware and software implementations of LFSRs are common. Read more about LFSRs in the **LFSR in Wikipedia** reference.
