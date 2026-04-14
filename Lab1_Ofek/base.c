#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

int main(int argc, char **argv){
  (void)argc; (void)argv;

  int base_len = 5;
  char arr1[base_len];
  char* arr2 = map(arr1,  base_len, my_get);
  char* arr3 = map(arr2,  base_len, dprt);
  char* arr4 = map(arr3,  base_len, cxprt);
  free(arr2);
  free(arr3);
  free(arr4);
  return 0;
}
