#include <stdio.h>
#include <string.h>

unsigned char password[] = "my_password1";
unsigned char *key = (unsigned char *)"A";
int direction = 1;
int keyPos = 0;

FILE *infile;
FILE *outfile;

int encode(int c) {
    int shift = key[keyPos] - 'A';
    if (c >= 'A' && c <= 'Z') {
        c = 'A' + (c - 'A' + direction * shift + 26) % 26;
        keyPos = key[keyPos + 1] ? keyPos + 1 : 0;
    } else if (c >= 'a' && c <= 'z') {
        c = 'a' + (c - 'a' + direction * shift + 26) % 26;
        keyPos = key[keyPos + 1] ? keyPos + 1 : 0;
    }
    return c;
}

int main(int argc, char **argv) {
    int debugMode = 1;
    int c;
    infile = stdin;
    outfile = stdout;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-D") == 0) {
            debugMode = 0;
        } else if (argv[i][0] == '+' && argv[i][1] == 'D') {
            if (strcmp(argv[i] + 2, (char *)password) == 0) {
                debugMode = 1;
            }
        } else if (argv[i][0] == '+' && argv[i][1] == 'V') {
            key = (unsigned char *)argv[i] + 2;
            direction = 1;
        } else if (argv[i][0] == '-' && argv[i][1] == 'V') {
            key = (unsigned char *)argv[i] + 2;
            direction = -1;
        }
        if (debugMode) {
            fprintf(stderr, "%s\n", argv[i]);
        }
    }

    while ((c = fgetc(infile)) != EOF) {
        fputc(encode(c), outfile);
    }

    fclose(outfile);
    return 0;
}
