#include <stdio.h>
#include <string.h>

const char password[] = "my_password1";
FILE *infile = NULL;
FILE *outfile = NULL;
const char *encoding_key = "A";
int encoding_sign = 1;
int encoding_pos = 0;

int encode(int c) {
    int shift;
    int base;

    if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        shift = encoding_key[encoding_pos] - 'A';
        shift *= encoding_sign;
        base = (c >= 'A' && c <= 'Z') ? 'A' : 'a';

        c = base + ((c - base + shift + 26) % 26);

        encoding_pos++;
        if(encoding_key[encoding_pos] == '\0') {
            encoding_pos = 0;
        }
    }

    return c;
}

int main(int argc, char *argv[]) {
    int debug = 1; // Default debug mode is on
    int c;

    infile = stdin;
    outfile = stdout;

    // Iterate over command line arguments
    for(int i = 0; i < argc; i++) {
        // If debug mode is on, print the argument to stderr
        if(debug) {
            fprintf(stderr, "%s\n", argv[i]);
        }

        // Check for debug flags to change mode for NEXT arguments
        if(strcmp(argv[i], "-D") == 0) {
            debug = 0;
        } else if(strncmp(argv[i], "+D", 2) == 0 && strcmp(argv[i] + 2, password) == 0) {
            debug = 1;
        } else if(strncmp(argv[i], "+V", 2) == 0) {
            encoding_key = argv[i] + 2;
            encoding_sign = 1;
            encoding_pos = 0;
        } else if(strncmp(argv[i], "-V", 2) == 0) {
            encoding_key = argv[i] + 2;
            encoding_sign = -1;
            encoding_pos = 0;
        }
    }

    // Input echoing loop
    // Read from stdin, simple encoding (none for now), write to stdout
    while(!feof(infile)) {
        c = fgetc(infile);
        if(c == EOF) {
            break;
        }
        c = encode(c);
        fputc(c, outfile);
    }

    fclose(outfile);

    return 0;
}
