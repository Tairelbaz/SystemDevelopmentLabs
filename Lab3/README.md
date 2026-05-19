# Lab 3: Assembly Language and System Calls

## Building

Requires WSL with `nasm`, `gcc` (multilib), and `ld`.

```bash
cd Lab3
make          # builds both task1 and task2
make task1    # builds only task1
make task2    # builds only task2
make clean    # removes all .o files and binaries
```

## Testing

### Task 1 — Vigenere Encoder

```bash
echo 'hello world' | ./task1 +VABC 2>/dev/null    # encode
echo 'hello' | ./task1 +VB -oout.txt 2>/dev/null  # output to file
./task1 +VB -iinput.txt 2>/dev/null                # input from file
```

### Task 2 — Virus Attacher

```bash
./task2              # list all files in current directory
./task2 -at          # infect files starting with 't'
```

**Warning:** always test task2 in a separate directory to avoid infecting source files.

## Local vs Submission Differences

The root directory files are configured for **WSL2 (64-bit kernel)**.
The submission zip (`213097306.zip`) is configured for **native 32-bit Linux** (lab machines).

The only difference is in `main.c` (task2):

| | Local (WSL2) | Submission (lab machines) |
|---|---|---|
| Syscall | `SYS_GETDENTS 220` (getdents64) | `SYS_GETDENTS 141` (getdents) |
| `d_ino` | `unsigned long long` (8 bytes) | `unsigned long` (4 bytes) |
| `d_off` | `unsigned long long` (8 bytes) | `unsigned long` (4 bytes) |
| `d_type` field | present | absent |

**Why:** WSL2 runs a 64-bit kernel where inode numbers exceed 32-bit range, causing `getdents` (141) to return `EOVERFLOW`. The 64-bit variant `getdents64` (220) with a larger struct resolves this. On native 32-bit Linux, syscall 141 works as expected.

All other files (assembly, makefiles, task1) are identical.
