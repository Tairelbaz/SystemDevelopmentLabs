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

#define TERMINATED  -1
#define RUNNING      1
#define SUSPENDED    0

typedef struct process {
    cmdLine* cmd;
    pid_t pid;
    int status;
    struct process *next;
} process;

process *process_list = NULL;

#define HISTLEN 10

typedef struct {
    int number;
    char cmdline[INPUT_MAX];
} histEntry;

histEntry history[HISTLEN];
int hist_start = 0;
int hist_count = 0;
int hist_next_num = 1;

int debug = 0;

void applyRedirections(cmdLine *pCmdLine) {
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
}

void addProcess(process **plist, cmdLine *cmd, pid_t pid) {
    process *p = malloc(sizeof(process));
    p->cmd = cmd;
    p->pid = pid;
    p->status = RUNNING;
    p->next = *plist;
    *plist = p;
}

void updateProcessStatus(process *plist, int pid, int status) {
    while (plist) {
        if (plist->pid == pid) {
            plist->status = status;
            return;
        }
        plist = plist->next;
    }
}

void updateProcessList(process **plist) {
    process *p = *plist;
    int wstatus;
    pid_t res;
    while (p) {
        if (p->status != TERMINATED) {
            res = waitpid(p->pid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);
            if (res > 0) {
                if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus))
                    p->status = TERMINATED;
                else if (WIFSTOPPED(wstatus))
                    p->status = SUSPENDED;
                else if (WIFCONTINUED(wstatus))
                    p->status = RUNNING;
            } else if (res == -1) {
                p->status = TERMINATED;
            }
        }
        p = p->next;
    }
}

void printProcessList(process **plist) {
    updateProcessList(plist);
    printf("PID          STATUS     Command\n");
    process *p = *plist;
    while (p) {
        const char *stat = p->status == RUNNING ? "Running" :
                           p->status == SUSPENDED ? "Suspended" : "Terminated";
        printf("%-13d%-11s", p->pid, stat);
        for (int i = 0; i < p->cmd->argCount; i++)
            printf("%s%s", p->cmd->arguments[i], i < p->cmd->argCount - 1 ? " " : "");
        printf("\n");
        p = p->next;
    }
    fflush(stdout);
    process **prev = plist;
    while (*prev) {
        if ((*prev)->status == TERMINATED) {
            process *dead = *prev;
            *prev = dead->next;
            freeCmdLines(dead->cmd);
            free(dead);
        } else {
            prev = &(*prev)->next;
        }
    }
}

void freeProcessList(process *plist) {
    while (plist) {
        process *next = plist->next;
        freeCmdLines(plist->cmd);
        free(plist);
        plist = next;
    }
}

void execute(cmdLine *pCmdLine) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        applyRedirections(pCmdLine);
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    if (debug)
        fprintf(stderr, "PID: %d\nExecuting command: %s\n%s\n",
                pid, pCmdLine->arguments[0],
                pCmdLine->blocking ? "Foreground" : "Background");

    addProcess(&process_list, pCmdLine, pid);

    if (pCmdLine->blocking)
        waitpid(pid, NULL, 0);
}

void executePipeline(cmdLine *pCmdLine) {
    if (pCmdLine->outputRedirect) {
        fprintf(stderr, "Error: output redirection on left side of pipe\n");
        freeCmdLines(pCmdLine);
        return;
    }
    if (pCmdLine->next->inputRedirect) {
        fprintf(stderr, "Error: input redirection on right side of pipe\n");
        freeCmdLines(pCmdLine);
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        freeCmdLines(pCmdLine);
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        freeCmdLines(pCmdLine);
        return;
    }

    if (pid1 == 0) {
        close(STDOUT_FILENO);
        dup(pipefd[1]);
        close(pipefd[1]);
        close(pipefd[0]);
        applyRedirections(pCmdLine);
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    close(pipefd[1]);

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork failed");
        close(pipefd[0]);
        freeCmdLines(pCmdLine);
        return;
    }

    if (pid2 == 0) {
        close(STDIN_FILENO);
        dup(pipefd[0]);
        close(pipefd[0]);
        applyRedirections(pCmdLine->next);
        execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments);
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    close(pipefd[0]);

    if (debug) {
        fprintf(stderr, "PID: %d\nExecuting command: %s\n", pid1, pCmdLine->arguments[0]);
        fprintf(stderr, "PID: %d\nExecuting command: %s\n", pid2, pCmdLine->next->arguments[0]);
    }

    cmdLine *secondCmd = pCmdLine->next;
    pCmdLine->next = NULL;
    addProcess(&process_list, pCmdLine, pid1);
    addProcess(&process_list, secondCmd, pid2);

    if (secondCmd->blocking) {
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
}

int execute_builtin(cmdLine *parsedLine) {
    if (strcmp(parsedLine->arguments[0], "quit") == 0) {
        freeCmdLines(parsedLine);
        freeProcessList(process_list);
        exit(0);
    }

    if (strcmp(parsedLine->arguments[0], "cd") == 0) {
        if (parsedLine->argCount < 2)
            fprintf(stderr, "cd: missing argument\n");
        else if (chdir(parsedLine->arguments[1]) == -1)
            fprintf(stderr, "cd failed: %s\n", strerror(errno));
        return 1;
    }

    if (strcmp(parsedLine->arguments[0], "procs") == 0) {
        printProcessList(&process_list);
        return 1;
    }

    if (strcmp(parsedLine->arguments[0], "stop") == 0) {
        if (parsedLine->argCount < 2)
            fprintf(stderr, "stop: missing process id\n");
        else {
            int pid = atoi(parsedLine->arguments[1]);
            if (kill(pid, SIGSTOP) == -1)
                perror("stop failed");
            else
                updateProcessStatus(process_list, pid, SUSPENDED);
        }
        return 1;
    }

    if (strcmp(parsedLine->arguments[0], "wakeup") == 0) {
        if (parsedLine->argCount < 2)
            fprintf(stderr, "wakeup: missing process id\n");
        else {
            int pid = atoi(parsedLine->arguments[1]);
            if (kill(pid, SIGCONT) == -1)
                perror("wakeup failed");
            else
                updateProcessStatus(process_list, pid, RUNNING);
        }
        return 1;
    }

    if (strcmp(parsedLine->arguments[0], "ice") == 0) {
        if (parsedLine->argCount < 2)
            fprintf(stderr, "ice: missing process id\n");
        else {
            int pid = atoi(parsedLine->arguments[1]);
            if (kill(pid, SIGINT) == -1)
                perror("ice failed");
            else
                updateProcessStatus(process_list, pid, TERMINATED);
        }
        return 1;
    }

    if (strcmp(parsedLine->arguments[0], "nuke") == 0) {
        if (parsedLine->argCount < 2)
            fprintf(stderr, "nuke: missing process id\n");
        else {
            int pid = atoi(parsedLine->arguments[1]);
            if (kill(-pid, SIGKILL) == -1)
                perror("nuke failed");
            else
                updateProcessStatus(process_list, pid, TERMINATED);
        }
        return 1;
    }

    return 0;
}

void addHistory(const char *cmdline) {
    int idx = (hist_start + hist_count) % HISTLEN;
    if (hist_count == HISTLEN)
        hist_start = (hist_start + 1) % HISTLEN;
    else
        hist_count++;
    strncpy(history[idx].cmdline, cmdline, INPUT_MAX - 1);
    history[idx].cmdline[INPUT_MAX - 1] = '\0';
    history[idx].number = hist_next_num++;
}

void printHistory(void) {
    for (int i = 0; i < hist_count; i++) {
        int idx = (hist_start + i) % HISTLEN;
        printf("%d  %s\n", history[idx].number, history[idx].cmdline);
    }
    fflush(stdout);
}

const char *getHistoryByNumber(int num) {
    for (int i = 0; i < hist_count; i++) {
        int idx = (hist_start + i) % HISTLEN;
        if (history[idx].number == num)
            return history[idx].cmdline;
    }
    return NULL;
}

const char *getLastNonHistoryCommand(void) {
    for (int i = hist_count - 1; i >= 0; i--) {
        int idx = (hist_start + i) % HISTLEN;
        const char *cmd = history[idx].cmdline;
        if (cmd[0] == '!' || strcmp(cmd, "history") == 0)
            continue;
        return cmd;
    }
    return NULL;
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
        fflush(stdout);

        if (fgets(input, INPUT_MAX, stdin) == NULL)
            break;

        if (strcmp(input, "\n") == 0)
            continue;

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "!!") == 0) {
            const char *recalled = getLastNonHistoryCommand();
            if (!recalled) {
                fprintf(stderr, "No commands in history\n");
                continue;
            }
            strncpy(input, recalled, INPUT_MAX - 1);
            input[INPUT_MAX - 1] = '\0';
        } else if (input[0] == '!' && input[1] >= '0' && input[1] <= '9') {
            int num = atoi(input + 1);
            const char *recalled = getHistoryByNumber(num);
            if (!recalled) {
                fprintf(stderr, "No such command in history: %d\n", num);
                continue;
            }
            strncpy(input, recalled, INPUT_MAX - 1);
            input[INPUT_MAX - 1] = '\0';
        } else if (strcmp(input, "history") == 0) {
            addHistory(input);
            printHistory();
            continue;
        }

        addHistory(input);

        parsedLine = parseCmdLines(input);
        if (parsedLine == NULL)
            continue;

        if (execute_builtin(parsedLine)) {
            freeCmdLines(parsedLine);
            continue;
        }

        if (parsedLine->next != NULL)
            executePipeline(parsedLine);
        else
            execute(parsedLine);
    }

    freeProcessList(process_list);
    return 0;
}
