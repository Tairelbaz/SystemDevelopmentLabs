#include <stdio.h>
#include <string.h>

// Part 1: Simple echo encoder
int encode(int c) {
    return c; // For now, just return the character unchanged
}

int main(int argc, char *argv[]) {
    int debug = 1; // Default debug mode is on
    int c;

    // Iterate over command line arguments
    for(int i = 1; i < argc; i++) {
        // If debug mode is on, print the argument to stderr
        if(debug) {
            fprintf(stderr, "%s\n", argv[i]);
        }

        // Check for debug flags to change mode for NEXT arguments
        if(strcmp(argv[i], "-D") == 0) {
            debug = 0;
        } else if(strncmp(argv[i], "+D", 2) == 0) {
            debug = 1;
        }
    }

    // Input echoing loop
    // Read from stdin, simple encoding (none for now), write to stdout
    while((c = fgetc(stdin)) != EOF) {
        c = encode(c);
        fputc(c, stdout);
    }

    return 0;
}
