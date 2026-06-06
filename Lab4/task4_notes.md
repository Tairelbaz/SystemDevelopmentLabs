# Lab 4 - Task 4 Notes

Reference notes for patching `ntsc` with a corrected `count_digits`.
Not part of the submission zip.

## Bug

`ntsc` is intended to count decimal digits in the input string, but its
`count_digits` function counts only `'1'` through `'8'`.

Examples before patching:
```sh
./ntsc 0123456789  # prints 8
./ntsc 0           # prints 0
./ntsc 9           # prints 0
```

The buggy comparisons are equivalent to excluding `'0'` and stopping at `'8'`
instead of accepting the full `'0'` through `'9'` range.

## Correct Function

`task4.c` defines a corrected function:
```c
int count_digits(char *s) {
  int count = 0;

  while (*s) {
    if (*s >= '0' && *s <= '9')
      count++;
    s++;
  }

  return count;
}
```

The `main` in `task4.c` prints the same output format as `ntsc`, so both
programs can be tested with the same inputs.

## Compile Flags

`task4` is built with:
```sh
gcc -m32 -Wall -g -fno-pie -no-pie -fno-stack-protector -o task4 task4.c
```

`-m32` makes the output compatible with the 32-bit `ntsc` binary.
`-fno-pie` and `-no-pie` keep the generated code non-position-independent.
`-fno-stack-protector` prevents canary code from adding an external failure
call inside the function.

## Restrictions On The Transplanted Function

The replacement `count_digits` must:

- Fit in the original `ntsc` function space: 93 bytes.
- Be self-contained: no library calls and no references to globals or string
  literals, because those addresses would belong to `task4`, not `ntsc`.
- Preserve the expected calling convention: receive `char *` as the first cdecl
  argument and return the `int` result in `eax`.

This is why only `count_digits` is transplanted. `main` is not suitable because
it calls library functions and uses external data.

## Patch Procedure

Known `ntsc` values:

- `count_digits` virtual address: `0x0804847d`
- original function size: 93 bytes
- `.text` file offset: `0x380`
- `.text` virtual address: `0x08048380`
- `count_digits` file offset: `0x380 + (0x0804847d - 0x08048380) = 0x47d`

After building `task4`, find its replacement `count_digits` address and size:
```sh
readelf -s task4
readelf -S task4
```

Then compute:
```text
task4_count_digits_file_offset = text_section_offset + (symbol_vaddr - text_section_vaddr)
```

Actual values from this build:

- `task4` `.text` virtual address: `0x08049060`
- `task4` `.text` file offset: `0x1060`
- `task4` `count_digits` virtual address: `0x08049176`
- `task4` `count_digits` size: 58 bytes
- `task4` `count_digits` file offset:
  `0x1060 + (0x08049176 - 0x08049060) = 0x1176`

Use `hexeditplus`:

```text
F
task4
U
1
L
1176 58
```

Pad the rest of the 93-byte buffer with `0x90` NOP bytes using `y`, then save
the complete 93-byte replacement region into `ntsc`:

```text
F
ntsc
S
0 47d 93
Q
```

## Verification

After patching:
```sh
./ntsc 0123456789        # prints 10
./ntsc 0                 # prints 1
./ntsc 9                 # prints 1
./ntsc aabbaba123baacca  # prints 3
./ntsc 1112111           # prints 7
```

The changed bytes are confined to the original function region: `ntsc` file
offsets `0x47d` through `0x47d + 92` (`0x4d9`). In this run, `cmp` showed 81
changed bytes, with the first changed byte at `0x485` and the last changed byte
at `0x4d9`; bytes before `0x485` in the function happened to match the original
prologue.
