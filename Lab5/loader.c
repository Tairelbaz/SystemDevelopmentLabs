/* Lab 5 - Tasks 0-2: a static ELF loader.
 * Authors: Ofek Hamdi, Tair Elbaz
 *
 * foreach_phdr (Task 0) iterates the program headers of an mmap'd 32-bit ELF.
 * print_phdr_info (Task 1) prints them readelf -l style with the mmap flags.
 * load_phdr (Task 2) maps each PT_LOAD segment to its virtual address; main
 * then hands control to the loaded program via startup() (from startup.s).
 *
 * Build (see makefile): linked with a custom linking_script that relocates the
 * loader to 0x04048000 so it does not collide with the loaded program's usual
 * 0x08048000, together with start.o (custom _start) and startup.o. libc is
 * linked dynamically -- the interpreter initialises it before _start runs.
 */
#include <stdio.h>      /* printf, fprintf, perror, fflush */
#include <stdlib.h>     /* exit                      */
#include <fcntl.h>      /* open, O_RDONLY            */
#include <unistd.h>     /* close                     */
#include <sys/stat.h>   /* fstat, struct stat        */
#include <sys/mman.h>   /* mmap, munmap, PROT_*, MAP_* */
#include <elf.h>        /* Elf32_Ehdr, Elf32_Phdr    */

/* Provided in startup.s: rebuilds the process stack with (argc, argv) for the
 * loaded program and jmps to its entry point. Does not return. */
extern int startup(int argc, char **argv, void (*start)());

/* Apply func to every program header in the ELF mapped at map_start.
 * Returns the number of program headers visited (e_phnum). */
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) map_start;
    /* The program-header table may live anywhere in the file: locate it via
     * e_phoff, never assume it follows the ELF header (e.g. loadme's e_phoff
     * is 10622, not 52). */
    char *phdr_base = (char *) map_start + ehdr->e_phoff;
    int i;

    for (i = 0; i < ehdr->e_phnum; i++) {
        /* Advance by e_phentsize, the file's declared entry stride, rather
         * than sizeof(Elf32_Phdr) -- spec-correct and future-proof. */
        Elf32_Phdr *phdr = (Elf32_Phdr *) (phdr_base + i * ehdr->e_phentsize);
        func(phdr, arg);
    }
    return ehdr->e_phnum;
}

/* Map a program-header p_type to the short name readelf prints. Unknown types
 * are formatted as hex into a small static buffer (so the returned pointer
 * stays valid after return). Covers the types any 32-bit Linux ELF may carry. */
static const char *phdr_type_name(Elf32_Word type) {
    switch (type) {
        case PT_NULL:         return "NULL";
        case PT_LOAD:         return "LOAD";
        case PT_DYNAMIC:      return "DYNAMIC";
        case PT_INTERP:       return "INTERP";
        case PT_NOTE:         return "NOTE";
        case PT_SHLIB:        return "SHLIB";
        case PT_PHDR:         return "PHDR";
        case PT_TLS:          return "TLS";
        case PT_GNU_EH_FRAME: return "GNU_EH_FRAME";
        case PT_GNU_STACK:    return "GNU_STACK";
        case PT_GNU_RELRO:    return "GNU_RELRO";
        case PT_GNU_PROPERTY: return "GNU_PROPERTY";
        default: {
            static char buf[16];
            snprintf(buf, sizeof(buf), "0x%x", type);
            return buf;
        }
    }
}

/* Task 1b: print, by symbolic name, the mmap protection flags implied by a
 * program header's p_flags. NOTE: the PF_* bits differ in position from the
 * PROT_* bits (PF_R=4 vs PROT_READ=1, PF_X=1 vs PROT_EXEC=4), so we translate
 * bit by bit rather than reuse p_flags as a prot value. */
static void print_mmap_prot_flags(Elf32_Word p_flags) {
    const char *sep = "";
    printf("    Protection (mmap): ");
    if (p_flags & PF_R) { printf("%sPROT_READ",  sep); sep = " | "; }
    if (p_flags & PF_W) { printf("%sPROT_WRITE", sep); sep = " | "; }
    if (p_flags & PF_X) { printf("%sPROT_EXEC",  sep); sep = " | "; }
    if (sep[0] == '\0') printf("PROT_NONE");
    printf("\n");
}

/* Task 2b helper: translate ELF p_flags (PF_R=4/PF_W=2/PF_X=1) into the mmap
 * protection bits (PROT_READ=1/PROT_WRITE=2/PROT_EXEC=4). The bit positions
 * differ, so we translate explicitly rather than reuse p_flags as a prot. */
static int phdr_to_mmap_prot(Elf32_Word p_flags) {
    int prot = PROT_NONE;
    if (p_flags & PF_R) prot |= PROT_READ;
    if (p_flags & PF_W) prot |= PROT_WRITE;
    if (p_flags & PF_X) prot |= PROT_EXEC;
    return prot;
}

/* Task 1 callback (reused by Task 2). The (Elf32_Phdr*, int) signature is fixed
 * by the spec; arg is unused here (it becomes the file descriptor in Task 2).
 * Task 1a: print every header's fields (readelf -l style). Task 1b: for headers
 * that get mapped (PT_LOAD), also print the mmap protection and mapping flags. */
static void print_phdr_info(Elf32_Phdr *phdr, int arg) {
    (void) arg;

    /* Task 1a: one row per header. */
    printf("%-7s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %c%c%c 0x%x\n",
           phdr_type_name(phdr->p_type),
           phdr->p_offset, phdr->p_vaddr, phdr->p_paddr,
           phdr->p_filesz, phdr->p_memsz,
           (phdr->p_flags & PF_R) ? 'R' : ' ',
           (phdr->p_flags & PF_W) ? 'W' : ' ',
           (phdr->p_flags & PF_X) ? 'E' : ' ',
           phdr->p_align);

    /* Task 1b: only segments that are actually mapped into memory. */
    if (phdr->p_type == PT_LOAD) {
        print_mmap_prot_flags(phdr->p_flags);
        printf("    Mapping (mmap): MAP_PRIVATE | MAP_FIXED\n");
    }
}

/* Task 2b: map one PT_LOAD segment from the file into its virtual address with
 * the segment's protection. arg is the file descriptor (passed through by
 * foreach_phdr). Non-PT_LOAD headers are skipped. Each mapped segment's info is
 * printed first, reusing the Task 1 printer. */
void load_phdr(Elf32_Phdr *phdr, int fd) {
    void *vaddr;
    off_t offset;
    unsigned int padding;
    void *map;

    if (phdr->p_type != PT_LOAD)
        return;

    print_phdr_info(phdr, fd);

    /* MAP_FIXED requires a page-aligned address and file offset. Round both
     * down to the page boundary and grow the length by the in-page padding so
     * the segment's bytes still land at its true p_vaddr. */
    vaddr   = (void *) (phdr->p_vaddr  & 0xfffff000);
    offset  = (off_t) (phdr->p_offset & 0xfffff000);
    padding = phdr->p_vaddr & 0xfff;

    /* Length is p_memsz (+padding), not p_filesz, so .bss is reserved; mmap
     * zero-fills the part of the final page that lies beyond the file data. */
    map = mmap(vaddr, phdr->p_memsz + padding,
               phdr_to_mmap_prot(phdr->p_flags),
               MAP_PRIVATE | MAP_FIXED, fd, offset);
    if (map == MAP_FAILED) {
        perror("mmap (load_phdr)");
        exit(1);
    }
}

int main(int argc, char **argv) {
    int fd;
    struct stat st;
    void *map_start;
    Elf32_Ehdr *ehdr;

    /* The target may take its own arguments: accept loader + program [+ ...]. */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <elf-file> [args...]\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    /* An empty file has no ELF header to read; mmap with length 0 fails
     * (EINVAL) anyway, so bail with a clear message. */
    if (st.st_size == 0) {
        fprintf(stderr, "%s: empty file\n", argv[1]);
        close(fd);
        return 1;
    }

    /* Read-only view of the file so we can read the ELF header and walk the
     * program-header table. The kernel picks a high address (no clash with the
     * 0x08048000 segments or the loader at 0x04048000). */
    map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    ehdr = (Elf32_Ehdr *) map_start;

    /* Task 2b: map every PT_LOAD segment to its virtual address. arg is now the
     * file descriptor so load_phdr can mmap from it. */
    printf("Type    Offset   VirtAddr   PhysAddr   FileSiz MemSiz Flg Align\n");
    foreach_phdr(map_start, load_phdr, fd);

    /* Flush our buffered output before handing over the CPU: the loaded program
     * exits via a raw int 0x80 and never flushes libc's buffers, so a
     * redirected/piped phdr report would otherwise be lost. */
    fflush(stdout);

    /* Hand control to the loaded program. Do NOT close(fd) or munmap the loaded
     * segments first -- the file must stay open and mapped while it runs.
     *
     * Task 2c (direct call; crashes encoder, which reads argv -- kept to show
     * why the loaded program needs a proper stack):
     *     ((void (*)()) ehdr->e_entry)();
     *
     * Task 2d: startup() builds the (argc, argv) stack the program's _start
     * expects, then jmps to the entry. argc-1 / argv+1 drop the loader's own
     * name so the program sees itself as argv[0]. startup() does not return. */
    startup(argc - 1, argv + 1, (void *) ehdr->e_entry);

    return 0;   /* not reached */
}
