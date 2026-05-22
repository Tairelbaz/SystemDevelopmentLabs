#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid1, pid2;

    char *cmd1[] = {"ps", "-xl", NULL};
    char *cmd2[] = {"grep", "5", NULL};

    /* Step 1: Create a pipe */
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    /* Step 2: Fork child1 */
    fprintf(stderr, "(parent_process>forking…)\n");
    pid1 = fork();

    if (pid1 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    /* Step 3: child1 — redirect stdout to pipe write-end, exec ps -xl */
    if (pid1 == 0) {
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
        close(STDOUT_FILENO);
        dup(pipefd[1]);
        close(pipefd[1]);
        close(pipefd[0]);
        fprintf(stderr, "(child1>going to execute cmd: ps -xl)\n");
        execvp(cmd1[0], cmd1);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>created process with id: %d)\n", pid1);

    /* Step 4: Parent closes write end */
    fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
    close(pipefd[1]);

    /* Step 5: Fork child2 */
    fprintf(stderr, "(parent_process>forking…)\n");
    pid2 = fork();

    if (pid2 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    /* Step 6: child2 — redirect stdin to pipe read-end, exec grep 5 */
    if (pid2 == 0) {
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
        close(STDIN_FILENO);
        dup(pipefd[0]);
        close(pipefd[0]);
        fprintf(stderr, "(child2>going to execute cmd: grep 5)\n");
        execvp(cmd2[0], cmd2);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>created process with id: %d)\n", pid2);

    /* Step 7: Parent closes read end */
    fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
    close(pipefd[0]);

    /* Step 8: Wait for children in creation order */
    fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    fprintf(stderr, "(parent_process>exiting…)\n");
    return 0;
}
