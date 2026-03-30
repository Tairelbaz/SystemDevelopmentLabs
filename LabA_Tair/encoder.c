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
    for(int i = 1; i < argc; i++) {
        // Apply debug toggles immediately.
        if(strcmp(argv[i], "-D") == 0) {
            debug = 0;
            continue;
        }
        if(strncmp(argv[i], "+D", 2) == 0 && strcmp(argv[i] + 2, password) == 0) {
            debug = 1;
            continue;
        }

        // If debug mode is on, print the argument to stderr
        if(debug) {
            fprintf(stderr, "%s\n", argv[i]);
        }

        if(strncmp(argv[i], "+V", 2) == 0) {
            encoding_key = argv[i] + 2;
            encoding_sign = 1;
            encoding_pos = 0;
        } else if(strncmp(argv[i], "-V", 2) == 0) {
            encoding_key = argv[i] + 2;
            encoding_sign = -1;
            encoding_pos = 0;
        } else if(strncmp(argv[i], "-i", 2) == 0) {
            infile = fopen(argv[i] + 2, "r");
            if(infile == NULL) {
                fprintf(stderr, "Error: failed opening input file %s\n", argv[i] + 2);
                return 1;
            }
        } else if(strncmp(argv[i], "-o", 2) == 0) {
            outfile = fopen(argv[i] + 2, "w");
            if(outfile == NULL) {
                fprintf(stderr, "Error: failed opening output file %s\n", argv[i] + 2);
                return 1;
            }
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

    if(infile != stdin) {
        fclose(infile);
    }

    if(outfile != stdout) {
        fclose(outfile);
    }

    return 0;
}
