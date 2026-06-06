# Lab 4 — Task 1c Analysis Answers

Reference notes for the two analysis questions embedded in Task 1c.
Not part of the submission zip — study material.

## Q1. Why may the printed hexadecimal values differ in order vs. `hexedit`?

Because of **little-endian** byte ordering on x86.

`hexedit` shows the raw bytes in **ascending address order** (byte 0, byte 1, …).
Memory Display instead reads `unit_size` bytes and interprets them as a single
**number**, which it prints most-significant digit first. On a little-endian machine
the least-significant byte is stored at the lowest address, so the number's bytes
appear **reversed** relative to the linear `hexedit` dump.

Example — bytes at file offset `0x12F` of `abc` are (ascending): `00 01 00 00 …`
- `hexedit` (bytes): `00 01 …`
- Memory Display, unit size 2: first unit = `0x0100` (= 256) — the `00` and `01`
  swapped, because `0x0100` little-endian is stored as `00 01`.

For unit size 1 there is no reordering (a single byte has no internal order).

## Q2. What is the entry point of your own `hexeditplus` program?

**Entry point address: `0x1150`.**

`hexeditplus` is built with the default `gcc -pie`, so `readelf -h` reports
`Type: DYN (Position-Independent Executable)`; the entry point is therefore a
relative address (`0x1150`), not a fixed virtual address like the non-PIE `abc`
(whose entry is `0x080483b0`).

Verified two ways:
```sh
# 1) directly from the ELF header
readelf -h hexeditplus          # -> Entry point address: 0x1150

# 2) with hexeditplus itself, reading its own e_entry (file offset 0x18, 4 bytes)
#    F=hexeditplus, U=4, L 18 1, M 0 1   ->  0x1150
printf 'F\nhexeditplus\nU\n4\nL\n18 1\nM\n0 1\nQ\n' | ./hexeditplus
```
Both report `0x1150`, confirming `e_entry` lives at file offset `0x18` of the ELF header.
