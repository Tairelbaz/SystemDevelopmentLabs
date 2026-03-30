#include <stdio.h>

int main(int argc, char const *argv[])
{
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if(i < argc - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}
