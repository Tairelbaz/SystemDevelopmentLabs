/* Lab 5 - Task 0: program-header iterator
 * Authors: Ofek Hamdi, Tair Elbaz
 *
 * Reads a 32-bit ELF executable (mmap'd read-only) and applies a callback
 * to each program header. Verified with a callback that prints each header's
 * index and in-memory address.
 */
#include <stdio.h>      /* printf, fprintf, perror   */
#include <fcntl.h>      /* open, O_RDONLY            */
#include <unistd.h>     /* close                     */
#include <sys/stat.h>   /* fstat, struct stat        */
#include <sys/mman.h>   /* mmap, munmap, PROT_*, MAP_* */
#include <elf.h>        /* Elf32_Ehdr, Elf32_Phdr    */

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

/* Verification callback for Task 0. The (Elf32_Phdr*, int) signature is fixed
 * by the spec; arg is unused here (it becomes the file descriptor in later
 * tasks), so a function-local static supplies the running index i. */
void print_phdr(Elf32_Phdr *phdr, int arg) {
    static int i = 0;
    (void) arg;
    printf("Program header number %d at address %p\n", i, (void *) phdr);
    i++;
}

int main(int argc, char **argv) {
    int fd;
    struct stat st;
    void *map_start;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
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

    map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    foreach_phdr(map_start, print_phdr, 0);

    munmap(map_start, st.st_size);
    close(fd);
    return 0;
}
