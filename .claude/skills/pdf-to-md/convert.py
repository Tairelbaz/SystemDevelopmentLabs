#!/usr/bin/env python3
"""
PDF to Markdown converter.

Usage:
    python3 convert.py input.pdf output.md [--no-bidi] [--title "Custom Title"]

Features:
    - Auto-detects RTL text and applies bidi correction
    - Detects numbered section headers → markdown headings
    - Converts bullet points (●, •, ○) → markdown lists
    - Detects code blocks via heuristics → fenced code blocks
    - Extracts tables → markdown pipe tables
    - Strips standalone page numbers
"""

import argparse
import re
import sys
from pathlib import Path

import pdfplumber


# ---------------------------------------------------------------------------
# RTL / Bidi helpers
# ---------------------------------------------------------------------------

# Hebrew: ֐-׿, Arabic: ؀-ۿ ݐ-ݿ ࢠ-ࣿ
_RTL_RE = re.compile(r'[֐-׿؀-ۿݐ-ݿࢠ-ࣿ]')


def _has_rtl(text: str) -> bool:
    """Return True if text contains RTL characters."""
    return bool(_RTL_RE.search(text))


def _apply_bidi(text: str) -> str:
    """Apply the Unicode bidi algorithm to fix visual → logical order."""
    try:
        from bidi.algorithm import get_display
        lines = text.split('\n')
        return '\n'.join(get_display(line) for line in lines)
    except ImportError:
        print("WARNING: python-bidi not installed. RTL text may appear reversed.", file=sys.stderr)
        print("  Install with: pip install python-bidi --break-system-packages", file=sys.stderr)
        return text


# ---------------------------------------------------------------------------
# Text cleaning
# ---------------------------------------------------------------------------

def _clean_line(line: str) -> str:
    """Remove common PDF extraction artifacts from a single line."""
    # Remove (cid:N) placeholders (unmapped glyphs)
    line = re.sub(r'\(cid:\d+\)', '', line)
    # Collapse multiple spaces (but preserve leading indent)
    stripped = line.lstrip()
    indent = line[:len(line) - len(stripped)]
    stripped = re.sub(r'  +', ' ', stripped)
    return indent + stripped.strip()


def _is_page_number(line: str) -> bool:
    """Return True if the line is just a standalone page number."""
    return bool(re.match(r'^\d{1,4}$', line.strip()))


# ---------------------------------------------------------------------------
# Code block detection
# ---------------------------------------------------------------------------

_CODE_KEYWORDS = re.compile(
    r'^(import |from |def |class |function |const |let |var |return |if \(|for \(|while \(|'
    r'public |private |static |void |int |string |boolean |console\.|print\(|'
    r'module\.exports|export |require\(|#include)',
)
_BRACE_LINE = re.compile(r'^[\s{}()\[\];]*$')


def _looks_like_code(line: str) -> bool:
    """Heuristic: does this line look like source code?"""
    s = line.strip()
    if not s:
        return False
    if _CODE_KEYWORDS.match(s):
        return True
    # Lines ending with { or }; or starting with }
    if s.endswith(('{', '};', '});')) or s.startswith('}'):
        return True
    # Lines that are just braces / parens
    if _BRACE_LINE.match(s):
        return True
    return False


# ---------------------------------------------------------------------------
# Section header detection
# ---------------------------------------------------------------------------

# Matches patterns like "1.2.3 Title text..."
_NUMBERED_HEADER = re.compile(r'^(\d+(?:\.\d+)+)\s+(.+)')
# Top-level: "1. Title" but NOT "1.2" style
_TOP_HEADER = re.compile(r'^(\d+)\.\s+([^\d].+)')


def _heading_depth(number_str: str) -> int:
    """Return heading level from a dotted number string."""
    return number_str.count('.') + 1


# ---------------------------------------------------------------------------
# Bullet point normalization
# ---------------------------------------------------------------------------

_BULLET_RE = re.compile(r'^[●•◦○]\s*(.+)')
_SUB_BULLET_RE = re.compile(r'^o\s+(.+)')


# ---------------------------------------------------------------------------
# Table extraction
# ---------------------------------------------------------------------------

def _table_to_markdown(table: list[list]) -> str:
    """Convert a pdfplumber table (list of rows) to a markdown pipe table."""
    if not table or not table[0]:
        return ''

    # Clean cells
    cleaned = []
    for row in table:
        cleaned.append([str(cell).strip() if cell else '' for cell in row])

    col_count = max(len(row) for row in cleaned)
    # Pad rows to uniform width
    for row in cleaned:
        while len(row) < col_count:
            row.append('')

    lines = []
    # Header
    lines.append('| ' + ' | '.join(cleaned[0]) + ' |')
    lines.append('| ' + ' | '.join('---' for _ in range(col_count)) + ' |')
    # Body
    for row in cleaned[1:]:
        lines.append('| ' + ' | '.join(row) + ' |')

    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# Main conversion
# ---------------------------------------------------------------------------

def extract_page(page, use_bidi: bool) -> dict:
    """Extract text and tables from a single pdfplumber page.

    Returns dict with 'text' (str) and 'tables' (list of markdown table strings).
    """
    text = page.extract_text() or ''
    if use_bidi and _has_rtl(text):
        text = _apply_bidi(text)

    tables_md = []
    try:
        tables = page.extract_tables()
        for t in tables:
            md = _table_to_markdown(t)
            if md:
                tables_md.append(md)
    except Exception:
        pass  # Tables are best-effort

    return {'text': text, 'tables': tables_md}


def format_lines(raw_text: str) -> list[str]:
    """Convert raw extracted text into markdown-formatted lines."""
    lines = raw_text.split('\n')
    output: list[str] = []
    in_code_block = False
    code_buffer: list[str] = []

    def flush_code():
        nonlocal in_code_block
        if code_buffer:
            # Try to guess language from content
            lang = ''
            joined = '\n'.join(code_buffer)
            if 'function ' in joined or 'const ' in joined or 'let ' in joined:
                lang = 'javascript'
            elif 'def ' in joined or 'import ' in joined or 'print(' in joined:
                lang = 'python'
            elif '#include' in joined:
                lang = 'c'
            output.append(f'```{lang}')
            output.extend(code_buffer)
            output.append('```')
            code_buffer.clear()
        in_code_block = False

    for line in lines:
        cleaned = _clean_line(line)

        # Skip page numbers
        if _is_page_number(cleaned):
            continue

        # Empty line
        if not cleaned:
            if in_code_block:
                code_buffer.append('')
            else:
                output.append('')
            continue

        # Code detection
        if _looks_like_code(cleaned):
            in_code_block = True
            code_buffer.append(cleaned)
            continue
        elif in_code_block:
            # Continuation: if the line is indented, keep it in the code block
            if line.startswith('  ') or line.startswith('\t'):
                code_buffer.append(cleaned)
                continue
            else:
                flush_code()

        # Numbered section headers (e.g. "1.2.3 Title")
        m = _NUMBERED_HEADER.match(cleaned)
        if m:
            flush_code()
            num, title = m.group(1), m.group(2)
            depth = _heading_depth(num)
            level = min(depth + 1, 6)  # cap at h6
            prefix = '#' * level
            output.append(f'\n{prefix} {num} {title}\n')
            continue

        # Top-level header (e.g. "1. Introduction")
        m = _TOP_HEADER.match(cleaned)
        if m:
            flush_code()
            num, title = m.group(1), m.group(2)
            output.append(f'\n# {num}. {title}\n')
            continue

        # Bullet points
        bm = _BULLET_RE.match(cleaned)
        if bm:
            output.append(f'- {bm.group(1)}')
            continue

        # Check for ● anywhere in line (common in bidi-reversed lines)
        if '●' in cleaned or '•' in cleaned:
            text = cleaned.replace('●', '').replace('•', '').strip()
            if text:
                output.append(f'- {text}')
                continue

        # Sub-bullets
        sm = _SUB_BULLET_RE.match(cleaned)
        if sm:
            output.append(f'  - {sm.group(1)}')
            continue

        # Regular line
        output.append(cleaned)

    # Flush any remaining code
    flush_code()

    return output


def convert_pdf_to_md(
    pdf_path: str,
    output_path: str,
    use_bidi: bool = True,
    title: str | None = None,
):
    """Convert a PDF file to a Markdown file."""
    pdf_path = Path(pdf_path)
    if not pdf_path.exists():
        print(f"ERROR: File not found: {pdf_path}", file=sys.stderr)
        sys.exit(1)

    with pdfplumber.open(str(pdf_path)) as pdf:
        total_pages = len(pdf.pages)
        print(f"Processing {total_pages} pages from {pdf_path.name}...")

        # Auto-detect RTL from first few pages
        if use_bidi:
            sample = ''
            for p in pdf.pages[:3]:
                sample += (p.extract_text() or '')
            if not _has_rtl(sample):
                use_bidi = False
                print("No RTL text detected, skipping bidi processing.")
            else:
                print("RTL text detected, applying bidi correction.")

        all_md_lines: list[str] = []

        # Add title
        if title:
            all_md_lines.append(f'# {title}\n')
            all_md_lines.append('---\n')

        for i, page in enumerate(pdf.pages):
            result = extract_page(page, use_bidi)

            # Format text lines
            page_lines = format_lines(result['text'])
            all_md_lines.extend(page_lines)

            # Append tables (if any and not already captured in text)
            for table_md in result['tables']:
                all_md_lines.append('')
                all_md_lines.append(table_md)
                all_md_lines.append('')

            # Page separator
            all_md_lines.append('')

            if (i + 1) % 20 == 0:
                print(f"  ...processed {i + 1}/{total_pages} pages")

    # Join and clean up excessive blank lines
    md_text = '\n'.join(all_md_lines)
    md_text = re.sub(r'\n{4,}', '\n\n\n', md_text)
    md_text = md_text.strip() + '\n'

    output = Path(output_path)
    output.write_text(md_text, encoding='utf-8')
    print(f"Done. Wrote {len(md_text):,} characters to {output}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description='Convert PDF to Markdown')
    parser.add_argument('input', help='Input PDF file path')
    parser.add_argument('output', help='Output Markdown file path')
    parser.add_argument('--no-bidi', action='store_true',
                        help='Disable RTL bidi correction even if RTL text is detected')
    parser.add_argument('--title', type=str, default=None,
                        help='Set a custom title for the markdown document')
    args = parser.parse_args()

    convert_pdf_to_md(
        pdf_path=args.input,
        output_path=args.output,
        use_bidi=not args.no_bidi,
        title=args.title,
    )


if __name__ == '__main__':
    main()
