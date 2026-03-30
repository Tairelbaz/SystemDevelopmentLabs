---
name: package
description: "Package a lab project directory into a zip file for submission. Triggered when the user says /package, 'package', 'zip', 'submit', or 'create submission zip'."
argument-hint: <project-dir>
allowed-tools: [Read, Glob, Grep, Bash]
---

# Package Lab Project for Submission

Package a lab project folder into a submission-ready zip file.

## Input

`$ARGUMENTS` is the project directory name (e.g. `LabA_Ofek`). It must be a directory in the repo root.

## Procedure

1. **Validate** the directory exists at the repo root. If not, list available directories and stop.

2. **Read instructions** — check if `instructions.pdf` exists in the project directory. If it does, read it and look for a "Submission" or "Deliverables" section that specifies which files to include. Extract the exact list of required files.

3. **Determine files to include** using these rules, in priority order:
   - If the instructions specify an explicit deliverables list (e.g. "A zip file containing exactly: makefile and encoder.c"), use **only** those files.
   - Otherwise, include all source code files (`.c`, `.h`, `.s`, `.asm`) and build files (`makefile`, `Makefile`, `CMakeLists.txt`), excluding:
     - Compiled artifacts: `*.o`, `*.out`, executables (files with no extension that are ELF binaries)
     - Documents: `*.pdf`, `*.doc`, `*.docx`, `*.txt`, `*.md`
     - Editor/IDE files: `*.swp`, `*.swo`, `*~`, `.vscode/`, `.idea/`
     - Archives: `*.zip`, `*.tar`, `*.gz`
     - Hidden files/directories

4. **List the files** that will be included and confirm with the user before proceeding.

5. **Create the zip** inside the project directory, named `<project-dir>.zip`. Overwrite if it already exists.

   Use a cross-platform approach:
   - Try `zip` first (Linux/WSL): `cd <project-dir> && zip -j <project-dir>.zip <files>`
   - If `zip` is not available, try `python3 -m zipfile -c` or `powershell.exe Compress-Archive`
   - Use `-j` (junk paths) so the zip contains flat files, not nested directories — unless instructions say otherwise.

6. **Verify** the zip was created and list its contents to confirm correctness.

## Important

- The zip must contain ONLY the required files — no extra files, no directory nesting (unless specified).
- Always prefer the explicit deliverables list from the instructions over the default rules.
- If `make clean` target exists, run it before packaging to ensure no stale build artifacts are present.
- Do NOT include the zip file itself in the zip.
