# Lab 4 — Task 0a Answers (file: `abc`)

Reference notes for the analysis questions on the provided ELF executable `abc`.
Not part of the submission zip — study material.

## Q1. Where is the entry point specified, and what is its value?

- **Where:** in the **ELF header**, field `e_entry`. For a 32-bit ELF it sits at
  **file offset `0x18`** (decimal 24), 4 bytes, little-endian.
- **Value (for `abc`):** `0x080483b0`.

Raw bytes at offset `0x18`: `b0 83 04 08` → little-endian → `0x080483b0`.
This equals the start address of the `.text` section (where execution begins).

## Q2. How many sections are there in `abc`?

- **Where:** ELF header field `e_shnum`, at **file offset `0x30`** (decimal 48),
  2 bytes, little-endian.
- **Value:** `1d 00` → `0x001d` → **29 sections**.

## Reproduce

```sh
# Native readelf is not installed on this Windows box; use WSL's binutils.
wsl readelf -h Lab4/abc      # "Entry point address" + "Number of section headers"
wsl readelf -S Lab4/abc      # full section list (also shows .text size = 0x20c)

# Raw-byte fallback (no readelf needed):
xxd -s 24 -l 4 Lab4/abc      # e_entry  -> b0 83 04 08  = 0x080483b0
xxd -s 48 -l 2 Lab4/abc      # e_shnum  -> 1d 00        = 29
```

## Bonus context (from `readelf -S`)

- `.text` section: size `0x20c`, addr `0x080483b0`, file offset `0x3b0`.
- Section header table offset (`e_shoff`, at file offset `0x20`): `0x113c`.
