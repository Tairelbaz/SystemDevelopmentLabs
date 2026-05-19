#include "util.h"

#define SYS_WRITE 4
#define STDOUT 1

extern int system_call();

int main(int argc, char *argv[], char *envp[])
{
  int i;
  for (i = 0; i < argc; i++) {
    system_call(SYS_WRITE, STDOUT, argv[i], strlen(argv[i]));
    system_call(SYS_WRITE, STDOUT, "\n", 1);
  }
  return 0;
}
