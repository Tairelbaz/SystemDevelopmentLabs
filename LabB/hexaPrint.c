#include <stdio.h>
#include <stdlib.h>

void PrintHex(char *buffer, long length) {
    for (long i = 0; i < length; i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    if(argc != 2){
        return 1; // error
    }
    char *inputFileName = argv[1];
    FILE *inputFile = fopen(inputFileName, "rb");
    if (inputFile == NULL) {
        fprintf(stderr, "Error opening file\n");
        return 1; // error
    }
    fseek(inputFile, 0, SEEK_END);
    long length = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);
    char *buffer = malloc(length);
    fread(buffer, 1, length, inputFile);

    PrintHex(buffer, length);

    free(buffer);
    fclose(inputFile);
    return 0;
    
}