#include <stdlib.h>
#include <stdio.h>

struct fun_desc {
  char *name;
  char index;
  char (*fun)(char);
};

char* map(char *array, int array_length, char (*f)(char)) { //code from moodle
  int i;
  char *mapped_array = (char*)malloc(array_length * sizeof(char));

  if (mapped_array == NULL)
    return NULL;

  for (i = 0; i < array_length; i++)
    mapped_array[i] = f(array[i]);

  return mapped_array;
}

char my_get(char c)
{
  (void)c;
  return (char)fgetc(stdin);
}

char dprt(char c)
{
  printf("%u\n", (unsigned char)c);
  return c;
}

char cxprt(char c)
{
  if (c >= 0x20 && c <= 0x7E)
    printf("%c %02x\n", c, (unsigned char)c);
  else
    printf(". %02x\n", (unsigned char)c);

  return c;
}

char encrypt(char c)
{
  unsigned char uc = (unsigned char)c;

  if (uc >= 0x20 && uc <= 0x7F) {
    if (uc == 0x7F)
      return 0x20;
    return c + 1;
  }

  return c;
}

char decrypt(char c)
{
  unsigned char uc = (unsigned char)c;

  if (uc >= 0x20 && uc <= 0x7F) {
    if (uc == 0x20)
      return 0x7F;
    return c - 1;
  }

  return c;
}

struct fun_desc menu[] = {
  { "<g>et string", 'g', my_get },
  { "<d>ecimal", 'd', dprt },
  { "<c>har and hex", 'c', cxprt },
  { "<e>ncrypt", 'e', encrypt },
  { "<x>decrypt", 'x', decrypt },
  { NULL, 0, NULL }
};

int main(void)
{
  char line[128];
  char carray_init[5] = "";
  char *carray = carray_init;
  int carray_heap = 0;

  while (1) {
    int i;
    int selected = -1;
    char *new_array;

    printf("Select operation from the following menu:\n");
    for (i = 0; menu[i].name != NULL; i++)
      printf("%s\n", menu[i].name);

    printf("Option: ");
    if (fgets(line, sizeof(line), stdin) == NULL) {
      printf("\n");
      break;
    }

    for (i = 0; menu[i].name != NULL; i++) {
      if (line[0] == menu[i].index) {
        selected = i;
        break;
      }
    }

    if (selected < 0) {
      printf("function not supported\n\n");
      continue;
    }

    new_array = map(carray, 5, menu[selected].fun);
    if (new_array == NULL) {
      if (carray_heap)
        free(carray);
      return 1;
    }

    if (carray_heap)
      free(carray);
    carray = new_array;
    carray_heap = 1;

    printf("\n");
  }

  if (carray_heap)
    free(carray);

  return 0;
}
