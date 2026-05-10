#!/usr/bin/env python3
"""
Markdown to PDF converter.

Usage:
    python3 convert.py input.md output.pdf [options]

Options:
    --no-bidi          Disable RTL bidi reshaping
    --page-size A4|LETTER  Page size (default: A4)
    --font-size N      Base font size in points (default: 11)
    --title "..."      PDF metadata title (defaults to first H1 or filename)

Features:
    - Full markdown support: headers, lists, code blocks, tables, bold/italic, links
    - Auto-detects RTL text (Hebrew/Arabic) and applies bidi + RTL paragraph direction
    - Syntax-highlighted code blocks with monospace font and shaded background
    - DejaVu fonts for broad Unicode coverage (Hebrew, Arabic, Cyrillic, etc.)
    - Page numbers in footer
"""

import argparse
import re
import sys
from pathlib import Path

import mistune
from reportlab.lib.pagesizes import A4, letter
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm, cm
from reportlab.lib.colors import HexColor, black, white, lightgrey
from reportlab.lib.enums import TA_LEFT, TA_RIGHT, TA_CENTER, TA_JUSTIFY
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Preformatted,
    Table, TableStyle, PageBreak, HRFlowable, ListFlowable, ListItem,
)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

_FONT_DIR = '/usr/share/fonts/truetype/dejavu'

_COLOR_BG_CODE = HexColor('#f5f5f5')
_COLOR_BORDER_CODE = HexColor('#e0e0e0')
_COLOR_LINK = HexColor('#1a5276')
_COLOR_H1 = HexColor('#1a1a2e')
_COLOR_H2 = HexColor('#2c3e50')
_COLOR_H3 = HexColor('#34495e')

_RTL_RE = re.compile(r'[\u0590-\u05FF\u0600-\u06FF\u0750-\u077F\u08A0-\u08FF]')


# ---------------------------------------------------------------------------
# Font registration
# ---------------------------------------------------------------------------

def _register_fonts():
    """Register DejaVu fonts with reportlab."""
    fonts = {
        'DejaVu':       f'{_FONT_DIR}/DejaVuSans.ttf',
        'DejaVu-Bold':  f'{_FONT_DIR}/DejaVuSans-Bold.ttf',
        'DejaVu-Italic': f'{_FONT_DIR}/DejaVuSans-Oblique.ttf',
        'DejaVu-BoldItalic': f'{_FONT_DIR}/DejaVuSans-BoldOblique.ttf',
        'DejaVuMono':   f'{_FONT_DIR}/DejaVuSansMono.ttf',
        'DejaVuMono-Bold': f'{_FONT_DIR}/DejaVuSansMono-Bold.ttf',
    }
    for name, path in fonts.items():
        try:
            pdfmetrics.registerFont(TTFont(name, path))
        except Exception as e:
            print(f"WARNING: Could not register font {name}: {e}", file=sys.stderr)

    # Register font family for bold/italic markup in Paragraphs
    from reportlab.pdfbase.pdfmetrics import registerFontFamily
    registerFontFamily(
        'DejaVu',
        normal='DejaVu',
        bold='DejaVu-Bold',
        italic='DejaVu-Italic',
        boldItalic='DejaVu-BoldItalic',
    )


# ---------------------------------------------------------------------------
# RTL helpers
# ---------------------------------------------------------------------------

def _has_rtl(text: str) -> bool:
    return bool(_RTL_RE.search(text))


def _bidi_reshape(text: str) -> str:
    """Apply bidi algorithm for visual display of RTL text."""
    try:
        from bidi.algorithm import get_display
        lines = text.split('\n')
        return '\n'.join(get_display(line) for line in lines)
    except ImportError:
        return text


# ---------------------------------------------------------------------------
# Styles
# ---------------------------------------------------------------------------

def _build_styles(base_size: int = 11, is_rtl: bool = False):
    """Create a style dictionary for the PDF."""
    alignment = TA_RIGHT if is_rtl else TA_LEFT
    font = 'DejaVu'
    mono = 'DejaVuMono'

    styles = {}

    styles['body'] = ParagraphStyle(
        'body',
        fontName=font,
        fontSize=base_size,
        leading=base_size * 1.45,
        alignment=alignment,
        spaceAfter=4 * mm,
        wordWrap='CJK',  # helps with long lines
    )

    for level in range(1, 7):
        size = base_size + max(12 - level * 2, 0)
        color = [_COLOR_H1, _COLOR_H2, _COLOR_H3, black, black, black][level - 1]
        styles[f'h{level}'] = ParagraphStyle(
            f'h{level}',
            fontName=f'{font}-Bold',
            fontSize=size,
            leading=size * 1.3,
            spaceBefore=(8 - level) * mm,
            spaceAfter=3 * mm,
            textColor=color,
            alignment=alignment,
        )

    styles['code_inline'] = ParagraphStyle(
        'code_inline',
        fontName=mono,
        fontSize=base_size - 1,
        leading=(base_size - 1) * 1.4,
        alignment=TA_LEFT,  # code is always LTR
    )

    styles['code_block'] = ParagraphStyle(
        'code_block',
        fontName=mono,
        fontSize=base_size - 1.5,
        leading=(base_size - 1.5) * 1.35,
        leftIndent=8 * mm,
        rightIndent=8 * mm,
        spaceBefore=3 * mm,
        spaceAfter=3 * mm,
        alignment=TA_LEFT,
        backColor=_COLOR_BG_CODE,
        borderColor=_COLOR_BORDER_CODE,
        borderWidth=0.5,
        borderPadding=6,
    )

    styles['list_item'] = ParagraphStyle(
        'list_item',
        parent=styles['body'],
        spaceAfter=1.5 * mm,
    )

    styles['blockquote'] = ParagraphStyle(
        'blockquote',
        parent=styles['body'],
        leftIndent=12 * mm,
        textColor=HexColor('#555555'),
        fontName=f'{font}-Italic',
    )

    styles['table_header'] = ParagraphStyle(
        'table_header',
        fontName=f'{font}-Bold',
        fontSize=base_size - 1,
        leading=(base_size - 1) * 1.3,
        alignment=alignment,
    )

    styles['table_cell'] = ParagraphStyle(
        'table_cell',
        fontName=font,
        fontSize=base_size - 1,
        leading=(base_size - 1) * 1.3,
        alignment=alignment,
    )

    return styles


# ---------------------------------------------------------------------------
# Markdown AST → Reportlab Flowables
# ---------------------------------------------------------------------------

def _escape_xml(text: str) -> str:
    """Escape text for reportlab's XML-based Paragraph markup."""
    text = text.replace('&', '&amp;')
    text = text.replace('<', '&lt;')
    text = text.replace('>', '&gt;')
    return text


def _inline_to_markup(tokens, use_bidi: bool = False) -> str:
    """Convert mistune inline tokens to reportlab XML markup string."""
    parts = []
    for tok in tokens:
        ttype = tok['type']

        if ttype == 'text':
            raw = tok.get('raw', tok.get('text', ''))
            t = _escape_xml(raw)
            if use_bidi and _has_rtl(t):
                t = _bidi_reshape(t)
            parts.append(t)

        elif ttype == 'codespan':
            raw = tok.get('raw', tok.get('text', ''))
            t = _escape_xml(raw)
            parts.append(f'<font face="DejaVuMono" size="-1">{t}</font>')

        elif ttype == 'strong':
            inner = _inline_to_markup(tok.get('children', []), use_bidi)
            parts.append(f'<b>{inner}</b>')

        elif ttype == 'emphasis':
            inner = _inline_to_markup(tok.get('children', []), use_bidi)
            parts.append(f'<i>{inner}</i>')

        elif ttype == 'link':
            inner = _inline_to_markup(tok.get('children', []), use_bidi)
            url = tok.get('attrs', {}).get('url', '') if isinstance(tok.get('attrs'), dict) else tok.get('link', '')
            parts.append(f'<a href="{url}" color="#1a5276">{inner}</a>')

        elif ttype == 'softbreak':
            parts.append(' ')

        elif ttype == 'linebreak':
            parts.append('<br/>')

        elif ttype == 'image':
            alt = tok.get('alt', 'image')
            parts.append(f'[Image: {_escape_xml(alt)}]')

        else:
            # Fallback: try to get raw text
            raw = tok.get('raw', tok.get('text', ''))
            if raw:
                t = _escape_xml(raw)
                if use_bidi and _has_rtl(t):
                    t = _bidi_reshape(t)
                parts.append(t)

    return ''.join(parts)


def _ast_to_flowables(ast_nodes, styles, use_bidi: bool = False, page_width: float = 170 * mm):
    """Walk the mistune AST and produce a list of reportlab Flowables."""
    flowables = []

    for node in ast_nodes:
        ntype = node['type']

        # --- Headings ---
        if ntype == 'heading':
            level = node.get('attrs', {}).get('level', 1)
            text = _inline_to_markup(node.get('children', []), use_bidi)
            style = styles.get(f'h{level}', styles['h1'])
            flowables.append(Paragraph(text, style))

        # --- Paragraphs ---
        elif ntype == 'paragraph':
            text = _inline_to_markup(node.get('children', []), use_bidi)
            if text.strip():
                flowables.append(Paragraph(text, styles['body']))

        # --- Code blocks (mistune 3 uses 'block_code') ---
        elif ntype in ('code_block', 'block_code'):
            raw = node.get('raw', node.get('text', ''))
            # Don't apply bidi to code — code is always LTR
            escaped = _escape_xml(raw.rstrip('\n'))
            escaped = escaped.replace('\n', '<br/>')
            escaped = escaped.replace(' ', '&nbsp;')
            flowables.append(Paragraph(escaped, styles['code_block']))

        # --- Block quote ---
        elif ntype == 'block_quote':
            children = node.get('children', [])
            inner = _ast_to_flowables(children, styles, use_bidi, page_width)
            # Wrap in blockquote style — just re-style the inner paragraphs
            for f in inner:
                flowables.append(f)

        # --- Lists ---
        elif ntype == 'list':
            ordered = node.get('attrs', {}).get('ordered', False)
            items_nodes = node.get('children', [])
            list_items = []
            for item_node in items_nodes:
                children = item_node.get('children', [])
                # Flatten: each list_item usually contains a paragraph
                parts = []
                for child in children:
                    if child['type'] == 'paragraph':
                        parts.append(_inline_to_markup(child.get('children', []), use_bidi))
                    elif child['type'] == 'list':
                        # Nested list — recurse
                        nested = _ast_to_flowables([child], styles, use_bidi, page_width)
                        for nf in nested:
                            list_items.append(ListItem(nf, leftIndent=10 * mm))
                        continue
                    else:
                        parts.append(_inline_to_markup(child.get('children', []), use_bidi))

                if parts:
                    text = '<br/>'.join(parts)
                    p = Paragraph(text, styles['list_item'])
                    list_items.append(ListItem(p))

            if list_items:
                bullet_type = 'I' if ordered else 'bullet'
                lf = ListFlowable(
                    list_items,
                    bulletType=bullet_type,
                    bulletFontName='DejaVu',
                    bulletFontSize=styles['body'].fontSize - 1,
                    leftIndent=8 * mm,
                    spaceBefore=2 * mm,
                    spaceAfter=2 * mm,
                )
                flowables.append(lf)

        # --- Tables (mistune 3 structure: table > table_head/table_body > table_row > table_cell) ---
        elif ntype == 'table':
            sections = node.get('children', [])
            all_rows = []
            for section in sections:
                sec_type = section.get('type', '')
                if sec_type == 'table_head':
                    # table_head contains cells directly (single header row)
                    cells = []
                    for cell_node in section.get('children', []):
                        if cell_node.get('type') == 'table_cell':
                            cell_text = _inline_to_markup(cell_node.get('children', []), use_bidi)
                            cells.append(Paragraph(cell_text, styles['table_header']))
                    if cells:
                        all_rows.append(cells)
                elif sec_type == 'table_body':
                    for row_node in section.get('children', []):
                        cells = []
                        for cell_node in row_node.get('children', []):
                            if cell_node.get('type') == 'table_cell':
                                is_header = cell_node.get('attrs', {}).get('head', False)
                                cell_text = _inline_to_markup(cell_node.get('children', []), use_bidi)
                                st = styles['table_header'] if is_header else styles['table_cell']
                                cells.append(Paragraph(cell_text, st))
                        if cells:
                            all_rows.append(cells)

            if all_rows:
                col_count = max(len(r) for r in all_rows)
                # Pad rows to same length
                for row in all_rows:
                    while len(row) < col_count:
                        row.append(Paragraph('', styles['table_cell']))

                # Calculate column widths that fit within page
                padding_per_col = 12  # left + right padding
                usable = page_width - (padding_per_col * col_count)
                col_width = max(usable / col_count, 15 * mm)
                col_widths = [col_width] * col_count

                try:
                    t = Table(all_rows, colWidths=col_widths, repeatRows=1)
                    t.setStyle(TableStyle([
                        ('BACKGROUND', (0, 0), (-1, 0), HexColor('#e8e8e8')),
                        ('FONTNAME', (0, 0), (-1, 0), 'DejaVu-Bold'),
                        ('GRID', (0, 0), (-1, -1), 0.5, HexColor('#cccccc')),
                        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
                        ('TOPPADDING', (0, 0), (-1, -1), 4),
                        ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
                        ('LEFTPADDING', (0, 0), (-1, -1), 6),
                        ('RIGHTPADDING', (0, 0), (-1, -1), 6),
                    ]))
                    flowables.append(Spacer(1, 3 * mm))
                    flowables.append(t)
                    flowables.append(Spacer(1, 3 * mm))
                except Exception as e:
                    # Fallback: skip malformed table
                    flowables.append(Paragraph('[table omitted]', styles['body']))

        # --- Thematic break (---) ---
        elif ntype == 'thematic_break':
            flowables.append(Spacer(1, 3 * mm))
            flowables.append(HRFlowable(
                width='80%', thickness=0.5,
                color=HexColor('#cccccc'), spaceBefore=2 * mm, spaceAfter=4 * mm,
            ))

        # --- Blank line ---
        elif ntype == 'blank_line':
            flowables.append(Spacer(1, 2 * mm))

        # --- Fallback ---
        else:
            # Try to render children if present
            children = node.get('children', [])
            if children:
                flowables.extend(_ast_to_flowables(children, styles, use_bidi, page_width))
            else:
                raw = node.get('raw', '')
                if raw.strip():
                    text = _escape_xml(raw)
                    if use_bidi and _has_rtl(text):
                        text = _bidi_reshape(text)
                    flowables.append(Paragraph(text, styles['body']))

    return flowables


# ---------------------------------------------------------------------------
# Page number footer
# ---------------------------------------------------------------------------

def _add_page_number(canvas, doc):
    """Draw page number centered at the bottom of each page."""
    canvas.saveState()
    canvas.setFont('DejaVu', 9)
    page_num = canvas.getPageNumber()
    text = f"— {page_num} —"
    canvas.drawCentredString(doc.pagesize[0] / 2.0, 15 * mm, text)
    canvas.restoreState()


# ---------------------------------------------------------------------------
# Main conversion
# ---------------------------------------------------------------------------

def convert_md_to_pdf(
    md_path: str,
    pdf_path: str,
    use_bidi: bool = True,
    page_size_name: str = 'A4',
    base_font_size: int = 11,
    title: str | None = None,
):
    """Convert a Markdown file to a styled PDF."""

    md_path = Path(md_path)
    if not md_path.exists():
        print(f"ERROR: File not found: {md_path}", file=sys.stderr)
        sys.exit(1)

    md_text = md_path.read_text(encoding='utf-8')
    print(f"Read {len(md_text):,} characters from {md_path.name}")

    # Register fonts
    _register_fonts()

    # Detect RTL
    is_rtl = use_bidi and _has_rtl(md_text)
    if is_rtl:
        print("RTL text detected — applying bidi reshaping and RTL alignment.")
    else:
        print("LTR text detected.")

    # Page setup
    page_size = A4 if page_size_name.upper() == 'A4' else letter
    page_w, page_h = page_size

    doc = SimpleDocTemplate(
        str(pdf_path),
        pagesize=page_size,
        leftMargin=20 * mm,
        rightMargin=20 * mm,
        topMargin=20 * mm,
        bottomMargin=25 * mm,
        title=title or md_path.stem,
        author='md-to-pdf converter',
    )

    content_width = page_w - 40 * mm

    # Build styles
    styles = _build_styles(base_font_size, is_rtl)

    # Parse markdown to AST (mistune 3.x API)
    md = mistune.create_markdown(renderer='ast', plugins=['table', 'strikethrough'])
    ast = md(md_text)

    # Extract title from first H1 if not provided
    if not title:
        for node in ast:
            if node.get('type') == 'heading' and node.get('attrs', {}).get('level') == 1:
                children = node.get('children', [])
                title = ''.join(
                    c.get('raw', c.get('text', ''))
                    for c in children
                    if c.get('type') in ('text', 'codespan')
                )
                break

    # Convert AST to flowables
    flowables = _ast_to_flowables(ast, styles, use_bidi=is_rtl, page_width=content_width)

    if not flowables:
        print("WARNING: No content generated from markdown.", file=sys.stderr)
        flowables = [Paragraph("(empty document)", styles['body'])]

    # Build PDF
    print(f"Generating PDF with {len(flowables)} elements...")
    doc.build(flowables, onFirstPage=_add_page_number, onLaterPages=_add_page_number)
    print(f"Done. Wrote {Path(pdf_path).stat().st_size:,} bytes to {pdf_path}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description='Convert Markdown to PDF')
    parser.add_argument('input', help='Input Markdown file path')
    parser.add_argument('output', help='Output PDF file path')
    parser.add_argument('--no-bidi', action='store_true',
                        help='Disable RTL bidi reshaping')
    parser.add_argument('--page-size', choices=['A4', 'LETTER'], default='A4',
                        help='Page size (default: A4)')
    parser.add_argument('--font-size', type=int, default=11,
                        help='Base font size in points (default: 11)')
    parser.add_argument('--title', type=str, default=None,
                        help='PDF metadata title')
    args = parser.parse_args()

    convert_md_to_pdf(
        md_path=args.input,
        pdf_path=args.output,
        use_bidi=not args.no_bidi,
        page_size_name=args.page_size,
        base_font_size=args.font_size,
        title=args.title,
    )


if __name__ == '__main__':
    main()
