#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    int pipefd[2];

    if (argc < 2) {
        fprintf(stderr, "Usage: mypipe <message>\n");
        return 1;
    }

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid = fork();

    if (pid == 0) {
        close(pipefd[1]);
        char buf[2048];
        int n = read(pipefd[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            write(STDOUT_FILENO, buf, n);
            write(STDOUT_FILENO, "\n", 1);
        }
        close(pipefd[0]);
        _exit(0);
    }

    close(pipefd[0]);
    write(pipefd[1], argv[1], strlen(argv[1]));
    close(pipefd[1]);
    waitpid(pid, NULL, 0);

    return 0;
}
