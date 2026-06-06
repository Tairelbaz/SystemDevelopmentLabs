# Lab 4 — Task 3 Answers (`offensive`)

Reference notes for displaying and NOP-ing out `main` in `offensive`.
Not part of the submission zip — study material.

## Finding the file offset of `main`

From the **symbol table** (`readelf -s offensive`):
- `main` — virtual address `0x0804841d`, size **23** bytes, section index **13** (`.text`).

From the **section table** (`readelf -S offensive`), section 13 = `.text`:
- `Addr` (virtual) = `0x08048320`, `Off` (file) = `0x320`.

**Formula:** `file_off = section_Off + (func_vaddr − section_Addr)`
```
file_off = 0x320 + (0x0804841d − 0x08048320)
         = 0x320 + 0xFD
         = 0x41D   (decimal 1053)
```
**Why it works:** a section is loaded as one contiguous block, so virtual address and file
offset differ by a constant within it. `func_vaddr − section_Addr` is the function's offset
*inside* the section; adding the section's file offset (`Off`) converts it to a position in the
file. (`offensive` is Type EXEC, so virtual addresses are absolute.)

## Displaying `main`'s compiled bytes (`hexeditplus`: F, U=1, L `41D 23`, M `0 23`)

23 bytes at file offset `0x41D`:
```
55 89 e5 83 e4 f0 83 ec 10 c7 04 24 d0 84 04 08 e8 be fe ff ff c9 c3
```
Disassembly:
```
push %ebp ; mov %esp,%ebp ; and $-16,%esp ; sub $0x10,%esp
movl $0x80484d0,(%esp) ; call puts@plt ; leave(c9) ; ret(c3)
```
Normal run prints: `Resistance is futile. / You will be assimilated! / (The Borg)`.

## Hacking it: NOP-out `main` (keep the `ret`)

Replace **all 22 bytes** of main from the prologue *through the `leave`* (file offsets
`0x41D`–`0x432`) with `NOP` (`0x90`), keeping only the final `ret` (`0xc3`) at `0x433`.

**Why this is safe:** `NOP` touches neither the stack nor registers, so when execution reaches
the surviving `ret`, `[esp]` still holds main's original return address and it returns cleanly to
`__libc_start_main`. The `leave` (`0xc9`) **must also become a NOP** — keeping it while NOPing
the prologue would run `mov esp,ebp; pop ebp` against a frame that was never set up, corrupting
the stack.

`hexeditplus` session (reusing the bytes already loaded by `L 41D 23`):
| Input | Effect |
|-------|--------|
| `U 4`; `y 0 90909090`, `y 4 90909090`, `y 8 90909090`, `y c 90909090`, `y 10 90909090` | NOP `mem_buf[0..19]` |
| `U 1`; `y 14 90`, `y 15 90` | NOP `mem_buf[20..21]` (keep `mem_buf[22]` = `c3`) |
| `S 0 41D 22` | write the 22 NOP bytes back to file `0x41D…0x432` |

## Verification
```
xxd -s 0x41D -l 23 offensive   # 90 (x22) c3
objdump -d offensive           # main: nop x22 ; ret
./offensive                    # no output, exit 0  (does nothing)
cmp -l <git-original> offensive # exactly 22 bytes differ (0x41D..0x432); ret + rest unchanged
```

## Alternative (the assignment's "alternately")
Plant a single `ret` at main's first byte: `U 1`, `y 0 c3`, `S 0 41D 1` — main returns
immediately. (We used the NOP-fill method above instead.)
