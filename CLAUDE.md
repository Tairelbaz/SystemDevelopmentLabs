# Systems Development Lab

## Project Overview

University course lab projects for **Systems Development Lab**. Contributors: **Ofek Hamdi** and **Tair Elbaz**.

## Repository Structure

```
LabX/          - Joint lab (both Ofek and Tair working together)
LabX_Ofek/     - Ofek's individual lab
LabX_Tair/     - Tair's individual lab
```

Where X is the lab letter (A, B, C, ...).

## Language & Tooling

- Primary languages: C, x86 Assembly (NASM, AT&T syntax)
- Build system: Make
- Platform: Linux (WSL2 on Windows)
- Architecture: x86 (32-bit) — use `gcc -m32` and `nasm -f elf32` where applicable

## Conventions

- Each lab folder is self-contained with its own makefile
- Assembly files use `.s` extension
- Follow the course assignment specifications for each lab
