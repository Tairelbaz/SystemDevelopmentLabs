# Lab 4 — Task 2 Answers (`deep_thought`)

Reference notes for fixing the buggy `deep_thought` executable.
Not part of the submission zip — study material.

## The bug

`deep_thought`'s ELF entry point (`e_entry`) was redirected to a function `hacked`
instead of the normal C-runtime entry `_start`, so the loader never runs the proper
startup path.

Disassembly (`objdump -d`):
- **`hacked` @ `0x08048464`** — `puts("This program has been hacked, …")`, `sleep(1)`,
  then `jmp` back: an infinite taunt loop. This was the (buggy) entry point.
- **`_start` @ `0x08048350`** — sets up `argc`/`argv`/`env` and calls
  `__libc_start_main(main, …)`.
- **`main` @ `0x0804844d`** — `puts("This program now working correctly, you win")`,
  then returns cleanly.

## Questions

- **Which "function" should precede `main` in execution?** `_start` — the kernel jumps
  to it first; it prepares the stack/arguments and calls `__libc_start_main`, which in
  turn calls `main`.
- **Virtual address of that function (`readelf -s`)?** `_start` = **`0x08048350`**.
- **Displaying the entry point with `hexeditplus` — what location/length, and how do we
  know?** location **`0x18`**, length **4 bytes** (one unit at unit size 4). In a 32-bit
  ELF the header is `e_ident[16] + e_type[2] + e_machine[2] + e_version[4]` = 24 bytes
  (`0x18`), where the 4-byte `Elf32_Addr e_entry` begins. (`deep_thought` is Type EXEC, so
  the address is absolute.)

## The fix (using `hexeditplus`, options L → y → S)

Overwrite the 4-byte `e_entry` at file offset `0x18` with `_start` (`0x08048350`). Since
Memory Modify writes the value little-endian, entering the address directly lands the
correct on-disk bytes — no manual byte-swap.

| Step | Input | Notes |
|------|-------|-------|
| `F` | `deep_thought` | target file |
| `U` | `4` | treat `e_entry` as one 4-byte unit |
| `L` | `18 1` | load the entry-point field into `mem_buf` |
| `M` | `0 1` | shows the buggy value `0x8048464` |
| `y` | `0 8048350` | set it to `_start` (writes LE `50 83 04 08`) |
| `S` | `0 18 1` | save the 4 bytes back to file offset `0x18` |
| `Q` | | |

## Verification

```
readelf -h deep_thought | grep Entry   # 0x8048464  ->  0x8048350
xxd -s 24 -l 4 deep_thought            # 50 83 04 08
./deep_thought                         # "This program now working correctly, you win", exit 0
```
`cmp` against the original shows the change is confined to the `e_entry` field
(`0x18`–`0x1B`); only 2 of the 4 bytes differ in value because both addresses share the
high half `0x0804` (bytes `04 08`).
