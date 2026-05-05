#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include "LineParser.h"

#define INPUT_MAX 2048

void execute(cmdLine *pCmdLine) {
    execvp(pCmdLine->arguments[0], pCmdLine->arguments);
    perror("execvp failed");
    _exit(EXIT_FAILURE);
}

int main() {
    char cwd[PATH_MAX];
    char input[INPUT_MAX];
    cmdLine *parsedLine;

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

        execute(parsedLine);
        freeCmdLines(parsedLine);
    }

    return 0;
}
