#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char my_get(char c);
char cxprt(char c);
char encrypt(char c);
char decrypt(char c);
char dprt(char c);
 
char* map(char *array, int array_length, char (*f) (char)){
  char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
  int i;

  if (mapped_array == NULL)
    return NULL;

  for (i = 0; i < array_length; i++)
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
  unsigned char uc = (unsigned char)c;

  if (uc >= 0x20 && uc <= 0x7F) {
    if (uc == 0x7F)
      return 0x20;
    return c + 1;
  }

  return c;
}

char decrypt(char c){
  unsigned char uc = (unsigned char)c;

  if (uc >= 0x20 && uc <= 0x7F) {
    if (uc == 0x20)
      return 0x7F;
    return c - 1;
  }

  return c;
}

char dprt(char c){
  printf("%u\n", (unsigned char)c);
  return c;
}
 
int main(int argc, char **argv){
  int base_len = 5;
  char arr1[5] = {0};
  char* arr2;
  char* arr3;
  char* arr4;
  char* arr5;
  char* arr6;

  (void)argc;
  (void)argv;

  arr2 = map(arr1, base_len, my_get);
  if (arr2 == NULL)
    return 1;

  arr3 = map(arr2, base_len, dprt);
  if (arr3 == NULL) {
    free(arr2);
    return 1;
  }

  arr4 = map(arr3, base_len, cxprt);
  if (arr4 == NULL) {
    free(arr2);
    free(arr3);
    return 1;
  }

  arr5 = map(arr2, base_len, encrypt);
  if (arr5 == NULL) {
    free(arr2);
    free(arr3);
    free(arr4);
    return 1;
  }

  arr6 = map(arr5, base_len, decrypt);
  if (arr6 == NULL) {
    free(arr2);
    free(arr3);
    free(arr4);
    free(arr5);
    return 1;
  }

  free(arr2);
  free(arr3);
  free(arr4);
  free(arr5);
  free(arr6);
  return 0;
}