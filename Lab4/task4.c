#include <stdio.h>

/* Self-contained so the compiled bytes can be transplanted into ntsc. */
int count_digits(char *s) {
  int count = 0;

  while (*s) {
    if (*s >= '0' && *s <= '9')
      count++;
    s++;
  }

  return count;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <string>\n", argv[0]);
    return 1;
  }

  printf("String contains %d digits, right???\n", count_digits(argv[1]));
  return 0;
}
