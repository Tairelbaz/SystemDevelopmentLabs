#include <stdio.h>
#include <string.h>

unsigned char password[] = "my_password1";
FILE *infile = NULL;
FILE *outfile = NULL;

// Part 1: Simple echo encoder
int encode(int c) {
    return c; // For now, just return the character unchanged
}

int main(int argc, char *argv[]) {
    int debug = 1; // Default debug mode is on
    int c;

    infile = stdin;
    outfile = stdout;

    // Iterate over command line arguments
    for(int i = 1; i < argc; i++) {
        // If debug mode is on, print the argument to stderr
        if(debug) {
            fprintf(stderr, "%s\n", argv[i]);
        }

        // Check for debug flags to change mode for NEXT arguments
        if(strcmp(argv[i], "-D") == 0) {
            debug = 0;
        } else if(strncmp(argv[i], "+D", 2) == 0 && strcmp(argv[i] + 2, (char *)password) == 0) {
            debug = 1;
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
