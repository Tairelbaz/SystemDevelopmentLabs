---
name: pdf-to-md
description: >
  Convert PDF files to well-formatted Markdown (.md) files. Use this skill whenever
  the user wants to convert a PDF to markdown, extract PDF content as markdown, turn
  lecture notes / slides / documents / papers / reports into .md files, or asks for
  a "markdown version" of a PDF. Also triggers for: "make this PDF into md",
  "extract this as markdown", "convert to markdown", "pdf to md", or any request
  that takes a .pdf as input and expects a .md as output. This skill handles
  right-to-left (Hebrew, Arabic) text, mixed LTR/RTL content, code blocks,
  tables, section headers, bullet lists, and multi-column layouts.
---

# PDF to Markdown Converter

## Overview

This skill converts PDF documents into clean, readable Markdown files. It handles:

- **RTL languages** (Hebrew, Arabic) via the `bidi` algorithm to fix reversed word order
- **Section headers** detected from numbering patterns and promoted to `#`/`##`/`###`
- **Bullet points** (`●`, `•`, `○`, `-`) normalized to markdown list syntax
- **Code blocks** detected heuristically and fenced with triple backticks
- **Tables** extracted via `pdfplumber` and converted to markdown pipe tables
- **Page numbers** stripped automatically
- **Multi-page documents** of any length

## Workflow

### Step 1: Inspect the PDF

Before converting, run a quick diagnostic to choose the right extraction strategy:

```bash
pdfinfo input.pdf
pdftotext -f 1 -l 1 input.pdf - | head -30
```

Check:
- **Page count** — affects processing time expectations
- **Text extractability** — does `pdftotext` return real text, or is it empty/garbled?
- **Language direction** — does the text contain Hebrew/Arabic characters? If yes, apply bidi fix.

### Step 2: Choose extraction strategy

| PDF type | Strategy |
|----------|----------|
| Text-heavy (reports, notes, papers) | `pdfplumber` text extraction → bidi fix if RTL → markdown formatting |
| Scanned / image-only | Rasterize with `pdftoppm` → OCR with `pytesseract` → markdown formatting |
| Slide decks | Rasterize pages → read visually → write markdown by hand |
| Mixed (text + figures) | Extract text, note figure locations, describe or skip figures |

For the common case (text-heavy PDFs), use the converter script at `.claude/skills/pdf-to-md/convert.py`.

### Step 3: Run the converter script

```bash
pip install pdfplumber python-bidi --break-system-packages -q
python3 .claude/skills/pdf-to-md/convert.py input.pdf output.md
```

The script auto-detects RTL content and applies bidi correction only when needed.

### Step 4: Review and post-process

After conversion, review the output for:

1. **Code blocks** — the script uses heuristics (indentation, braces, keywords) to detect code. Manually verify fencing is correct, especially in documents mixing code with natural language.
2. **Tables** — complex merged-cell tables may not convert perfectly. Check and fix manually if needed.
3. **Section hierarchy** — the script detects numbered headers (`1.1`, `1.1.1`, etc.). Documents with unnumbered headers or unconventional numbering may need manual adjustment.
4. **RTL mixed lines** — lines mixing Hebrew/Arabic with English code or technical terms can have odd word ordering. These are inherent to bidi text extraction from PDFs and may need spot fixes.

## Edge Cases and Troubleshooting

**Garbled text output**: Check `pdffonts input.pdf`. If fonts show "no" in the "emb" column or have "Custom" encoding, the PDF's character mapping is broken. Fall back to rasterize + OCR.

**Empty text extraction**: The PDF is likely scanned images. Use:
```bash
pdftoppm -jpeg -r 200 input.pdf /tmp/page
# Then OCR each page image
```

**Very large PDFs (500+ pages)**: The script processes all pages in memory. For huge documents, consider splitting first:
```bash
pdftk input.pdf cat 1-100 output part1.pdf
```

**Math / equations**: These rarely extract as readable text. Note their locations and consider describing them or leaving placeholders like `[equation]`.

## What the script does NOT do

- Extract or embed images (figures are skipped; a placeholder could be added)
- Preserve exact visual layout (markdown is semantic, not visual)
- Handle encrypted / password-protected PDFs (decrypt first with `qpdf`)
- OCR scanned documents (use `pytesseract` separately if needed)
