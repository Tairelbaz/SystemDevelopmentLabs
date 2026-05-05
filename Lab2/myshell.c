#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include "LineParser.h"

#define INPUT_MAX 2048

int debug = 0;

void execute(cmdLine *pCmdLine) {
    pid_t pid = fork();

    if (pid == 0) {
        if (pCmdLine->inputRedirect) {
            close(STDIN_FILENO);
            if (open(pCmdLine->inputRedirect, O_RDONLY) == -1) {
                perror("input redirection failed");
                _exit(EXIT_FAILURE);
            }
        }
        if (pCmdLine->outputRedirect) {
            close(STDOUT_FILENO);
            if (open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644) == -1) {
                perror("output redirection failed");
                _exit(EXIT_FAILURE);
            }
        }
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    if (debug)
        fprintf(stderr, "PID: %d\nExecuting command: %s\n%s\n",
                pid, pCmdLine->arguments[0],
                pCmdLine->blocking ? "Foreground" : "Background");

    if (pCmdLine->blocking)
        waitpid(pid, NULL, 0);
}

int main(int argc, char **argv) {
    char cwd[PATH_MAX];
    char input[INPUT_MAX];
    cmdLine *parsedLine;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)
            debug = 1;
    }

    while (1) {
        if (getcwd(cwd, PATH_MAX) == NULL) {
            perror("getcwd failed");
            exit(EXIT_FAILURE);
        }
        printf("%s> ", cwd);

        if (fgets(input, INPUT_MAX, stdin) == NULL)
            break;

        if (strcmp(input, "\n") == 0)
            continue;

        parsedLine = parseCmdLines(input);
        if (parsedLine == NULL)
            continue;

        if (strcmp(parsedLine->arguments[0], "quit") == 0) {
            freeCmdLines(parsedLine);
            break;
        }

        if (strcmp(parsedLine->arguments[0], "cd") == 0) {
            if (chdir(parsedLine->arguments[1]) == -1)
                fprintf(stderr, "cd failed: %s\n", strerror(errno));
            freeCmdLines(parsedLine);
            continue;
        }

        if (strcmp(parsedLine->arguments[0], "stop") == 0) {
            if (kill(atoi(parsedLine->arguments[1]), SIGSTOP) == -1)
                perror("stop failed");
            freeCmdLines(parsedLine);
            continue;
        }

        if (strcmp(parsedLine->arguments[0], "wakeup") == 0) {
            if (kill(atoi(parsedLine->arguments[1]), SIGCONT) == -1)
                perror("wakeup failed");
            freeCmdLines(parsedLine);
            continue;
        }

        if (strcmp(parsedLine->arguments[0], "ice") == 0) {
            if (kill(atoi(parsedLine->arguments[1]), SIGINT) == -1)
                perror("ice failed");
            freeCmdLines(parsedLine);
            continue;
        }

        if (strcmp(parsedLine->arguments[0], "nuke") == 0) {
            if (kill(-atoi(parsedLine->arguments[1]), SIGKILL) == -1)
                perror("nuke failed");
            freeCmdLines(parsedLine);
            continue;
        }

        execute(parsedLine);
        freeCmdLines(parsedLine);
    }

    return 0;
}
