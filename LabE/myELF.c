/* myELF.c - Lab E: a limited pass I (merge) of a linkage editor.
 *
 * Implemented: Part 0 (menu + Examine ELF File), Part 1 (Section Names),
 * Part 2a (Symbols), Part 2b (Relocations), Part 3.1 (Check Merge),
 * Part 3.2 (Merge), Part 3.3 (symbol resolution, bonus).
 *
 * All ELF file content is accessed through mmap (no read/fread); the merged
 * output file is produced with write().
 *
 * Contributors: Ofek Hamdi and Tair Elbaz.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <elf.h>

#define MAX_FILES 2

/* ---- per-file global state (we keep up to MAX_FILES open at once) ---- */
int    debug_mode = 0;
int    fd[MAX_FILES]        = { -1, -1 };
void  *map_start[MAX_FILES] = { NULL, NULL };
size_t map_size[MAX_FILES]  = { 0, 0 };
char   fname[MAX_FILES][256];
int    file_count = 0;

/* ---- menu option type (array-of-structures menu, as in lab 1) ---- */
struct menu_option {
  char  key;
  char *name;
  void (*func)(void);
};

/* ---- small helpers ---- */

/* Read a line from stdin into buf, strip the trailing newline.
 * Returns buf, or NULL on EOF. (stdin is console input, not ELF file
 * input, so using fgets here is allowed.) */
static char *read_line(char *buf, int size) {
  if (fgets(buf, size, stdin) == NULL)
    return NULL;
  buf[strcspn(buf, "\n")] = '\0';
  return buf;
}

static const char *data_encoding_str(unsigned char d) {
  switch (d) {
    case ELFDATA2LSB: return "2's complement, little endian";
    case ELFDATA2MSB: return "2's complement, big endian";
    default:          return "none / unknown";
  }
}

/* ---- ELF accessors over the mmap'd image of file `fi` ---- */

static Elf32_Ehdr *elf_hdr(int fi) {
  return (Elf32_Ehdr *)map_start[fi];
}

static Elf32_Shdr *sec_table(int fi) {
  return (Elf32_Shdr *)((char *)map_start[fi] + elf_hdr(fi)->e_shoff);
}

/* name of section `idx`, read from the section-header string table (.shstrtab) */
static char *sec_name(int fi, int idx) {
  Elf32_Ehdr *ehdr = elf_hdr(fi);
  Elf32_Shdr *sh   = sec_table(fi);
  char *shstr = (char *)map_start[fi] + sh[ehdr->e_shstrndx].sh_offset;
  return shstr + sh[idx].sh_name;
}

/* index of the first section of the given type, or -1 if none */
static int find_section_by_type(int fi, Elf32_Word type) {
  Elf32_Ehdr *ehdr = elf_hdr(fi);
  Elf32_Shdr *sh   = sec_table(fi);
  int i;
  for (i = 0; i < ehdr->e_shnum; i++)
    if (sh[i].sh_type == type)
      return i;
  return -1;
}

/* human-readable name of a symbol's section index, handling the special
 * reserved values (UND/ABS/COMMON) that are not real section indices */
static const char *shndx_name(int fi, unsigned shndx) {
  switch (shndx) {
    case SHN_UNDEF:  return "UND";
    case SHN_ABS:    return "ABS";
    case SHN_COMMON: return "COMMON";
  }
  if (shndx < (unsigned)elf_hdr(fi)->e_shnum)
    return sec_name(fi, shndx);
  return "?";
}

/* name of an i386 relocation type (the value extracted from r_info);
 * falls back to the raw number for any type we do not name explicitly */
static const char *reloc_type_str(unsigned t) {
  static char buf[16];
  switch (t) {
    case R_386_NONE:     return "R_386_NONE";
    case R_386_32:       return "R_386_32";
    case R_386_PC32:     return "R_386_PC32";
    case R_386_GOT32:    return "R_386_GOT32";
    case R_386_PLT32:    return "R_386_PLT32";
    case R_386_COPY:     return "R_386_COPY";
    case R_386_GLOB_DAT: return "R_386_GLOB_DAT";
    case R_386_JMP_SLOT: return "R_386_JMP_SLOT";
    case R_386_RELATIVE: return "R_386_RELATIVE";
    case R_386_GOTOFF:   return "R_386_GOTOFF";
    case R_386_GOTPC:    return "R_386_GOTPC";
    default:
      snprintf(buf, sizeof(buf), "%u", t);
      return buf;
  }
}

/* ---- helpers for check-merge and merge ---- */

static int sym_is_defined(Elf32_Sym *s) {
  return s->st_shndx != SHN_UNDEF;
}

/* find a named non-local symbol in file fi's symbol table (skips the dummy
 * symbol 0 and unnamed symbols). Local symbols are not visible to other ELF
 * files, so they must not satisfy cross-file undefined references. */
static Elf32_Sym *find_visible_symbol(int fi, const char *name) {
  Elf32_Shdr *sh = sec_table(fi);
  int sti = find_section_by_type(fi, SHT_SYMTAB);
  Elf32_Shdr *symtab;
  Elf32_Sym  *syms;
  char *strbase;
  int nsyms, i;

  if (sti < 0)
    return NULL;
  symtab  = &sh[sti];
  syms    = (Elf32_Sym *)((char *)map_start[fi] + symtab->sh_offset);
  nsyms   = symtab->sh_size / symtab->sh_entsize;
  strbase = (char *)map_start[fi] + sh[symtab->sh_link].sh_offset;

  for (i = 1; i < nsyms; i++) {
    const char *n = strbase + syms[i].st_name;
    if (ELF32_ST_BIND(syms[i].st_info) != STB_LOCAL &&
        n[0] != '\0' && strcmp(n, name) == 0)
      return &syms[i];
  }
  return NULL;
}

/* number of sections of a given type (used to require exactly one symtab) */
static int count_section_by_type(int fi, Elf32_Word type) {
  Elf32_Ehdr *ehdr = elf_hdr(fi);
  Elf32_Shdr *sh   = sec_table(fi);
  int i, c = 0;
  for (i = 0; i < ehdr->e_shnum; i++)
    if (sh[i].sh_type == type)
      c++;
  return c;
}

/* index of the section named `name` in file fi, or -1 if not present */
static int find_section_by_name(int fi, const char *name) {
  Elf32_Ehdr *ehdr = elf_hdr(fi);
  int i;
  for (i = 0; i < ehdr->e_shnum; i++)
    if (strcmp(sec_name(fi, i), name) == 0)
      return i;
  return -1;
}

/* sections whose contents are concatenated when merging */
static int is_mergeable(const char *name) {
  return strcmp(name, ".text")   == 0 ||
         strcmp(name, ".data")   == 0 ||
         strcmp(name, ".rodata") == 0;
}

/* write exactly n bytes to fd_out, handling short writes */
static int write_all(int fd_out, const void *buf, size_t n) {
  const char *p = (const char *)buf;
  while (n > 0) {
    ssize_t w = write(fd_out, p, n);
    if (w < 0) { perror("write"); return -1; }
    p += w;
    n -= (size_t)w;
  }
  return 0;
}

/* Part 3.3 (bonus): build a resolved copy of the first file's symbol table.
 * Every symbol that is UNDEFINED in file 0 but defined in file 1 receives
 * file 1's section (remapped to the merged file's index by name) and value
 * (shifted past file 0's own contribution to that merged section).
 * Returns a malloc'd buffer of *out_size bytes (caller frees), or NULL. */
static Elf32_Sym *build_resolved_symtab(Elf32_Shdr *symtab0, size_t *out_size) {
  Elf32_Shdr *sh0 = sec_table(0);
  Elf32_Sym  *src = (Elf32_Sym *)((char *)map_start[0] + symtab0->sh_offset);
  char *str0 = (char *)map_start[0] + sh0[symtab0->sh_link].sh_offset;
  int n = symtab0->sh_size / symtab0->sh_entsize;
  Elf32_Sym *dst;
  int i;

  dst = malloc(symtab0->sh_size);
  if (dst == NULL) { perror("malloc"); return NULL; }
  memcpy(dst, src, symtab0->sh_size);

  for (i = 1; i < n; i++) {                      /* skip the dummy symbol 0 */
    const char *name = str0 + dst[i].st_name;
    Elf32_Sym *f2;
    const char *secn;
    int mshndx;

    if (dst[i].st_shndx != SHN_UNDEF || name[0] == '\0')
      continue;                                  /* only resolve undefined  */
    f2 = find_visible_symbol(1, name);
    if (f2 == NULL || !sym_is_defined(f2))
      continue;                                  /* not defined in file 1   */

    secn   = sec_name(1, f2->st_shndx);          /* file1's section name    */
    mshndx = find_section_by_name(0, secn);      /* merged (=file0) index   */
    if (mshndx < 0)
      continue;

    dst[i].st_shndx = (Elf32_Half)mshndx;
    dst[i].st_value = f2->st_value +
                      (is_mergeable(secn) ? sh0[mshndx].sh_size : 0);
    dst[i].st_info  = f2->st_info;               /* carry type/binding over */

    if (debug_mode)
      fprintf(stderr, "[debug] resolved %s -> shndx=%d value=0x%x\n",
              name, mshndx, (unsigned)dst[i].st_value);
  }

  *out_size = symtab0->sh_size;
  return dst;
}

/* ---- menu actions ---- */

void toggle_debug(void) {
  debug_mode = !debug_mode;
  fprintf(stderr, "Debug flag now %s\n", debug_mode ? "on" : "off");
}

void examine_elf(void) {
  char path[256];
  int slot, f;
  struct stat st;
  void *start;
  Elf32_Ehdr *ehdr;

  if (file_count >= MAX_FILES) {
    fprintf(stderr, "Error: already have %d ELF files open\n", MAX_FILES);
    return;
  }

  printf("Enter ELF file name: ");
  if (read_line(path, sizeof(path)) == NULL || path[0] == '\0') {
    fprintf(stderr, "Error: no file name given\n");
    return;
  }

  f = open(path, O_RDONLY);
  if (f < 0) {
    perror("open");
    return;
  }

  if (fstat(f, &st) < 0) {
    perror("fstat");
    close(f);
    return;
  }
  if (st.st_size < (off_t)sizeof(Elf32_Ehdr)) {
    fprintf(stderr, "Error: %s is too small to be an ELF file\n", path);
    close(f);
    return;
  }

  /* map the whole file once, read-only */
  start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, f, 0);
  if (start == MAP_FAILED) {
    perror("mmap");
    close(f);
    return;
  }

  /* validate the ELF magic number (including the leading 0x7f byte) */
  ehdr = (Elf32_Ehdr *)start;
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
    fprintf(stderr, "Error: %s is not an ELF file (bad magic)\n", path);
    munmap(start, st.st_size);
    close(f);
    return;
  }

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
    fprintf(stderr, "Error: %s is not a 32-bit ELF file\n", path);
    munmap(start, st.st_size);
    close(f);
    return;
  }

  /* record this file in the next free slot */
  slot = file_count;
  fd[slot]        = f;
  map_start[slot] = start;
  map_size[slot]  = st.st_size;
  strncpy(fname[slot], path, sizeof(fname[slot]) - 1);
  fname[slot][sizeof(fname[slot]) - 1] = '\0';
  file_count++;

  if (debug_mode)
    fprintf(stderr, "[debug] mapped %s: fd=%d addr=%p size=%ld (slot %d)\n",
            path, f, start, (long)st.st_size, slot);

  /* print header information in the order required by the assignment */
  printf("Magic bytes 1,2,3: %c%c%c\n",
         ehdr->e_ident[1], ehdr->e_ident[2], ehdr->e_ident[3]);
  printf("Data encoding: %s\n", data_encoding_str(ehdr->e_ident[EI_DATA]));
  printf("Entry point: 0x%x\n", (unsigned)ehdr->e_entry);
  printf("Section header table offset: %u\n", (unsigned)ehdr->e_shoff);
  printf("Number of section headers: %u\n", (unsigned)ehdr->e_shnum);
  printf("Size of section header entry: %u\n", (unsigned)ehdr->e_shentsize);
  printf("Program header table offset: %u\n", (unsigned)ehdr->e_phoff);
  printf("Number of program headers: %u\n", (unsigned)ehdr->e_phnum);
  printf("Size of program header entry: %u\n", (unsigned)ehdr->e_phentsize);
}

void print_section_names(void) {
  int fi, i;

  if (file_count == 0) {
    fprintf(stderr, "Error: no ELF file is open\n");
    return;
  }

  for (fi = 0; fi < file_count; fi++) {
    Elf32_Ehdr *ehdr = elf_hdr(fi);
    Elf32_Shdr *sh   = sec_table(fi);
    int shnum = ehdr->e_shnum;

    if (debug_mode)
      fprintf(stderr,
              "[debug] %s: e_shoff=%u e_shnum=%u e_shstrndx=%u shstrtab_off=%u\n",
              fname[fi], (unsigned)ehdr->e_shoff, (unsigned)ehdr->e_shnum,
              (unsigned)ehdr->e_shstrndx,
              (unsigned)sh[ehdr->e_shstrndx].sh_offset);

    printf("File %s sections:\n", fname[fi]);
    for (i = 0; i < shnum; i++) {
      if (debug_mode)
        fprintf(stderr, "[debug]   [%2d] sh_name offset=%u\n",
                i, (unsigned)sh[i].sh_name);
      printf("[%2d] %-18s %08x %08x %08x  %2u\n",
             i, sec_name(fi, i),
             (unsigned)sh[i].sh_addr, (unsigned)sh[i].sh_offset,
             (unsigned)sh[i].sh_size, (unsigned)sh[i].sh_type);
    }
    printf("\n");
  }
}

void print_symbols(void) {
  int fi, i;

  if (file_count == 0) {
    fprintf(stderr, "Error: no ELF file is open\n");
    return;
  }

  for (fi = 0; fi < file_count; fi++) {
    Elf32_Shdr *sh = sec_table(fi);
    int symtab_idx = find_section_by_type(fi, SHT_SYMTAB);
    Elf32_Shdr *symtab;
    Elf32_Sym  *syms;
    char *strbase;
    int nsyms;

    if (symtab_idx < 0) {
      fprintf(stderr, "File %s: no symbol table\n", fname[fi]);
      continue;
    }

    symtab  = &sh[symtab_idx];
    syms    = (Elf32_Sym *)((char *)map_start[fi] + symtab->sh_offset);
    nsyms   = symtab->sh_size / symtab->sh_entsize;
    strbase = (char *)map_start[fi] + sh[symtab->sh_link].sh_offset;  /* .strtab */

    if (debug_mode)
      fprintf(stderr,
              "[debug] %s: symtab=[%d] size=%u entsize=%u nsyms=%d strtab=[%u]\n",
              fname[fi], symtab_idx, (unsigned)symtab->sh_size,
              (unsigned)symtab->sh_entsize, nsyms, (unsigned)symtab->sh_link);

    printf("File %s symbols:\n", fname[fi]);
    for (i = 0; i < nsyms; i++) {
      Elf32_Sym *s = &syms[i];
      printf("[%2d] %08x %5u %-12s %s\n",
             i, (unsigned)s->st_value, (unsigned)s->st_shndx,
             shndx_name(fi, s->st_shndx), strbase + s->st_name);
    }
    printf("\n");
  }
}

void print_relocations(void) {
  int fi, s, i;

  if (file_count == 0) {
    fprintf(stderr, "Error: no ELF file is open\n");
    return;
  }

  for (fi = 0; fi < file_count; fi++) {
    Elf32_Shdr *sh = sec_table(fi);
    int shnum = elf_hdr(fi)->e_shnum;
    char *base = (char *)map_start[fi];
    int relidx = 0, found = 0;

    printf("File %s relocations:\n", fname[fi]);

    /* a file may hold several relocation sections (.rel.text, .rel.data, ...) */
    for (s = 0; s < shnum; s++) {
      Elf32_Shdr *rel = &sh[s];
      Elf32_Shdr *symtab;
      Elf32_Sym  *syms;
      char *strbase;
      int count;

      if (rel->sh_type != SHT_REL && rel->sh_type != SHT_RELA)
        continue;
      found = 1;

      symtab  = &sh[rel->sh_link];                       /* associated symtab */
      syms    = (Elf32_Sym *)(base + symtab->sh_offset);
      strbase = base + sh[symtab->sh_link].sh_offset;    /* its .strtab       */
      count   = rel->sh_size / rel->sh_entsize;

      if (debug_mode)
        fprintf(stderr,
                "[debug] %s: %s entries=%d entsize=%u symtab=[%u] strtab=[%u]\n",
                fname[fi], sec_name(fi, s), count,
                (unsigned)rel->sh_entsize, (unsigned)rel->sh_link,
                (unsigned)symtab->sh_link);

      for (i = 0; i < count; i++) {
        char *entry = base + rel->sh_offset + (size_t)i * rel->sh_entsize;
        Elf32_Addr r_offset = ((Elf32_Rel *)entry)->r_offset;
        Elf32_Word r_info   = ((Elf32_Rel *)entry)->r_info;  /* same slot in Rela */
        Elf32_Sym *rs = &syms[ELF32_R_SYM(r_info)];
        const char *nm = strbase + rs->st_name;

        if (nm[0] == '\0')                /* section symbol: use the section name */
          nm = shndx_name(fi, rs->st_shndx);

        printf("[%2d] %08x  %-14s %s\n",
               relidx, (unsigned)r_offset, nm,
               reloc_type_str(ELF32_R_TYPE(r_info)));
        relidx++;
      }
    }

    if (!found)
      printf("No relocations\n");
    printf("\n");
  }
}

/* Part 3.1: scan file `a`'s global symbols against file `b`, report errors */
static void check_one_direction(int a, int b) {
  Elf32_Shdr *sh = sec_table(a);
  int sti = find_section_by_type(a, SHT_SYMTAB);
  Elf32_Shdr *symtab = &sh[sti];
  Elf32_Sym  *syms = (Elf32_Sym *)((char *)map_start[a] + symtab->sh_offset);
  int nsyms = symtab->sh_size / symtab->sh_entsize;
  char *strbase = (char *)map_start[a] + sh[symtab->sh_link].sh_offset;
  int i;

  for (i = 1; i < nsyms; i++) {                 /* skip the dummy symbol 0 */
    Elf32_Sym *s = &syms[i];
    const char *name = strbase + s->st_name;
    Elf32_Sym *other;

    /* only global symbols take part in cross-module resolution */
    if (ELF32_ST_BIND(s->st_info) == STB_LOCAL || name[0] == '\0')
      continue;

    other = find_visible_symbol(b, name);

    if (!sym_is_defined(s)) {                    /* undefined in file a */
      if (other == NULL || !sym_is_defined(other))
        printf("Symbol %s undefined\n", name);
    } else {                                     /* defined in file a */
      if (other != NULL && sym_is_defined(other))
        printf("Symbol %s multiply defined\n", name);
    }
  }
}

void check_merge(void) {
  if (file_count < 2) {
    fprintf(stderr, "Error: two ELF files must be open to check for merge\n");
    return;
  }
  if (count_section_by_type(0, SHT_SYMTAB) != 1 ||
      count_section_by_type(1, SHT_SYMTAB) != 1) {
    printf("feature not supported\n");
    return;
  }
  check_one_direction(0, 1);
  check_one_direction(1, 0);
}

/* Part 3.2: merge the two open ELF files into "out.ro" */
void merge(void) {
  Elf32_Ehdr  ehdr;
  Elf32_Shdr *sh0, *newsh;
  int shnum, out, i;
  long cur;

  if (file_count < 2) {
    fprintf(stderr, "Error: two ELF files must be open to merge\n");
    return;
  }

  ehdr  = *elf_hdr(0);                 /* copy of first file's ELF header   */
  sh0   = sec_table(0);
  shnum = ehdr.e_shnum;

  newsh = malloc(shnum * sizeof(Elf32_Shdr));   /* copy of first file's SHT */
  if (newsh == NULL) { perror("malloc"); return; }
  memcpy(newsh, sh0, shnum * sizeof(Elf32_Shdr));

  out = open("out.ro", O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (out < 0) { perror("open out.ro"); free(newsh); return; }

  /* 1. write an initial ELF header (e_shoff fixed up at the end) */
  if (write_all(out, &ehdr, sizeof(ehdr)) < 0) goto fail;
  cur = sizeof(ehdr);

  /* 2-3. process each section in the first file's section-header order */
  for (i = 0; i < shnum; i++) {
    char *name = sec_name(0, i);
    char *src0 = (char *)map_start[0] + sh0[i].sh_offset;
    Elf32_Word size0 = sh0[i].sh_size;

    if (i == 0) {                       /* NULL section: nothing to write */
      newsh[i].sh_offset = 0;
      continue;
    }

    newsh[i].sh_offset = (Elf32_Off)cur;

    if (is_mergeable(name)) {            /* concatenate file0 then file1 */
      int j = find_section_by_name(1, name);
      if (write_all(out, src0, size0) < 0) goto fail;
      cur += size0;
      if (j >= 0) {
        Elf32_Shdr *sh1 = sec_table(1);
        char *src1 = (char *)map_start[1] + sh1[j].sh_offset;
        if (write_all(out, src1, sh1[j].sh_size) < 0) goto fail;
        cur += sh1[j].sh_size;
        newsh[i].sh_size = size0 + sh1[j].sh_size;
      }
    } else if (sh0[i].sh_type == SHT_SYMTAB) {  /* Part 3.3: resolved copy */
      size_t sz;
      Elf32_Sym *resolved = build_resolved_symtab(&sh0[i], &sz);
      if (resolved == NULL) goto fail;
      if (write_all(out, resolved, sz) < 0) { free(resolved); goto fail; }
      free(resolved);
      cur += sz;
    } else if (sh0[i].sh_type != SHT_NOBITS && size0 > 0) {
      if (write_all(out, src0, size0) < 0) goto fail;   /* copy as-is */
      cur += size0;
    }

    if (debug_mode)
      fprintf(stderr, "[debug] [%d] %-10s off=%u size=%u\n",
              i, name, (unsigned)newsh[i].sh_offset, (unsigned)newsh[i].sh_size);
  }

  /* 4. write the new section header table, remember where it landed */
  ehdr.e_shoff = (Elf32_Off)cur;
  if (write_all(out, newsh, shnum * sizeof(Elf32_Shdr)) < 0) goto fail;

  /* 5. fix e_shoff by rewriting the header, then close */
  if (lseek(out, 0, SEEK_SET) < 0) { perror("lseek"); goto fail; }
  if (write_all(out, &ehdr, sizeof(ehdr)) < 0) goto fail;

  close(out);
  free(newsh);
  printf("Merged 2 files into out.ro\n");
  return;

fail:
  close(out);
  free(newsh);
}

void not_implemented(void) {
  printf("not implemented yet\n");
}

void quit(void) {
  int i;
  for (i = 0; i < file_count; i++) {
    if (map_start[i] != NULL && map_start[i] != MAP_FAILED)
      munmap(map_start[i], map_size[i]);
    if (fd[i] >= 0)
      close(fd[i]);
  }
  if (debug_mode)
    fprintf(stderr, "[debug] cleaned up %d file(s), exiting\n", file_count);
  exit(0);
}

struct menu_option menu[] = {
  { 'D', "Toggle >D<ebug Mode",     toggle_debug    },
  { 'F', "Examine ELF >F<ile",      examine_elf     },
  { 'N', "Print Section >N<ames",   print_section_names },
  { 'S', "Print >S<ymbols",         print_symbols   },
  { 'R', "Print >R<elocations",     print_relocations },
  { 'C', ">C<heck Files for Merge", check_merge     },
  { 'M', ">M<erge ELF Files",       merge           },
  { 'Q', ">Q<uit",                  quit            },
  { 0, NULL, NULL }
};

int main(void) {
  char line[256];
  int i;

  while (1) {
    printf("Choose action:\n");
    for (i = 0; menu[i].name != NULL; i++)
      printf("%s\n", menu[i].name);
    printf("Option: ");

    if (read_line(line, sizeof(line)) == NULL)  /* EOF -> clean exit */
      quit();
    if (line[0] == '\0')
      continue;

    /* dispatch on the first character (case-insensitive) */
    for (i = 0; menu[i].name != NULL; i++) {
      if (toupper((unsigned char)line[0]) == menu[i].key) {
        menu[i].func();
        break;
      }
    }
    if (menu[i].name == NULL)
      printf("Unknown option: %s\n", line);

    printf("\n");
  }
  return 0;
}
