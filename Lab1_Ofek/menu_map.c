#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CARRAY_LEN 5
#define INPUT_LEN  256

char* map(char *array, int array_length, char (*f)(char)){
  char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
  for (int i = 0; i < array_length; i++)
    mapped_array[i] = f(array[i]);
  return mapped_array;
}

char my_get(char c){
  (void)c;
  return (char)fgetc(stdin);
}

char cxprt(char c){
  if (c >= 0x20 && c <= 0x7E)
    printf("%c %02x\n", c, (unsigned char)c);
  else
    printf(". %02x\n", (unsigned char)c);
  return c;
}

char encrypt(char c){
  if (c == 0x7F) return 0x20;
  if (c >= 0x20 && c <= 0x7E) return c + 1;
  return c;
}

char decrypt(char c){
  if (c == 0x20) return 0x7F;
  if (c > 0x20 && c <= 0x7F) return c - 1;
  return c;
}

char dprt(char c){
  printf("%d\n", (unsigned char)c);
  return c;
}

struct fun_desc {
  char *name;
  char index;
  char (*fun)(char);
};

int main(int argc, char **argv){
  (void)argc; (void)argv;

  char *carray = calloc(CARRAY_LEN, sizeof(char));

  struct fun_desc menu[] = {
    { "<g> Get String",          'g', my_get  },
    { "<d> Print Decimal",       'd', dprt    },
    { "<c> Print Char and Hex",  'c', cxprt   },
    { "<e> Encrypt",             'e', encrypt },
    { "<x> Decrypt",             'x', decrypt },
    { NULL, 0, NULL }
  };

  char input[INPUT_LEN];

  while (1) {
    printf("Select operation from the following menu:\n");
    for (int i = 0; menu[i].name != NULL; i++)
      printf("  %s\n", menu[i].name);

    if (fgets(input, sizeof(input), stdin) == NULL)
      break;

    char chosen = input[0];

    int found = -1;
    for (int i = 0; menu[i].name != NULL; i++) {
      if (menu[i].index == chosen) {
        found = i;
        break;
      }
    }

    if (found == -1) {
      printf("function not supported\n");
      continue;
    }

    char *new_carray = map(carray, CARRAY_LEN, menu[found].fun);
    free(carray);
    carray = new_carray;
  }

  free(carray);
  return 0;
}
