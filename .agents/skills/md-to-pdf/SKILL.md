---
name: md-to-pdf
description: >
  Convert Markdown (.md) files to well-formatted PDF documents. Use this skill
  whenever the user wants to turn a markdown file into a PDF, render markdown as
  a PDF, generate a printable version of a .md file, or asks for a "PDF version"
  of markdown content. Also triggers for: "make this md into a pdf", "convert
  markdown to pdf", "md to pdf", "render as pdf", "export to pdf", or any
  request that takes .md as input and expects a .pdf as output. This skill
  handles right-to-left (Hebrew, Arabic) text, code blocks with monospace font
  and shading, markdown tables, headers, lists, bold/italic, links, blockquotes,
  and horizontal rules. Uses DejaVu fonts for broad Unicode coverage.
---

# Markdown to PDF Converter

## Overview

This skill converts Markdown files into clean, professionally styled PDF documents using `reportlab` and `mistune`. It produces PDFs with:

- **Styled headers** (H1-H6) with distinct sizes and colors
- **Body text** in DejaVu Sans with proper leading and spacing
- **Code blocks** in DejaVu Sans Mono with a shaded background
- **Tables** with header row highlighting and grid lines
- **Lists** (ordered and unordered, including nested)
- **Bold, italic, inline code, and links**
- **RTL language support** (Hebrew, Arabic) with automatic bidi detection
- **Page numbers** in the footer

## Dependencies

The script requires `mistune`, `reportlab`, and optionally `python-bidi` for RTL:

```bash
pip install mistune reportlab python-bidi --break-system-packages -q
```

DejaVu fonts must be available at `/usr/share/fonts/truetype/dejavu/` (standard on Ubuntu/Debian). If missing:

```bash
apt-get install -y fonts-dejavu-core
```

## Usage

### Basic conversion

```bash
python3 .Codex/skills/md-to-pdf/convert.py input.md output.pdf
```

### Options

| Flag | Description | Default |
|------|-------------|---------|
| `--no-bidi` | Disable RTL bidi reshaping even if RTL is detected | auto-detect |
| `--page-size A4\|LETTER` | Page size | A4 |
| `--font-size N` | Base font size in points | 11 |
| `--title "..."` | PDF metadata title | first H1 or filename |

### Examples

```bash
# Hebrew lecture notes, A4
python3 .Codex/skills/md-to-pdf/convert.py lecture.md lecture.pdf

# English report, US Letter, larger font
python3 .Codex/skills/md-to-pdf/convert.py report.md report.pdf --page-size LETTER --font-size 12

# Force LTR even if RTL characters are present
python3 .Codex/skills/md-to-pdf/convert.py mixed.md mixed.pdf --no-bidi
```

## How It Works

1. **Parse** the markdown into an AST using `mistune` (with table and strikethrough plugins)
2. **Detect RTL** by scanning the first few pages for Hebrew/Arabic Unicode ranges
3. **Walk the AST** and convert each node into a reportlab Flowable:
   - Headings -> `Paragraph` with bold font and scaled sizes
   - Paragraphs -> `Paragraph` with body style
   - Code blocks -> `Paragraph` with monospace font, grey background, and border
   - Lists -> `ListFlowable` with `ListItem` children
   - Tables -> `Table` with `TableStyle` for grid, header highlighting
   - Blockquotes -> `Paragraph` with italic font and left indent
   - Thematic breaks -> `HRFlowable`
4. **Build the PDF** with `SimpleDocTemplate`, adding page numbers via canvas callbacks

## RTL Handling

When Hebrew or Arabic characters are detected in the markdown:

- The `python-bidi` library applies the Unicode Bidirectional Algorithm to fix visual ordering
- Paragraph alignment switches to right-aligned (`TA_RIGHT`)
- Code blocks remain left-aligned (code is always LTR)
- Mixed LTR/RTL lines (e.g., Hebrew text with English variable names) are handled by the bidi algorithm, though some edge cases in heavily mixed content may produce imperfect ordering

## Limitations

- **Images**: Markdown image references (`![alt](url)`) are rendered as `[Image: alt]` text placeholders. Embedding images would require downloading/locating the files and is not currently supported.
- **Math / LaTeX**: Not supported. Equations render as raw markup text.
- **Custom CSS / styling**: The PDF uses a fixed style set. Custom colors, fonts, or margins require editing the script's `_build_styles` function.
- **Very wide tables**: Tables with many columns are auto-shrunk to fit the page width. Extremely wide tables may have cramped cells.
- **Nested blockquotes**: Only one level of blockquote indentation is applied.

## Troubleshooting

**Font errors**: If you see "WARNING: Could not register font", ensure DejaVu fonts are installed. Run `fc-list | grep -i dejavu` to verify.

**Bidi import error**: Install `python-bidi` with `pip install python-bidi --break-system-packages`. Without it, RTL text will appear with reversed word order.

**Table errors**: If a table causes a crash, the script falls back to a `[table omitted]` placeholder. This can happen with malformed markdown tables or tables with empty rows.

**Large documents**: The script processes the entire markdown in memory. For very large files (10MB+), memory usage may be significant but should still work on typical systems.
