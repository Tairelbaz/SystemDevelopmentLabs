#include <stdio.h>

unsigned char password[] = "my_password1";
unsigned char debugModeFlagOff[] = "-D";
unsigned char debugModeFlagOn[] = "+Dpassword";

int main(int argc, char **argv) {
    int debugMode = 1;
    
    for (int i = 1; i < argc; i++)
    {
        if(argv[i]==debugModeFlagOff){
            debugMode = 0;
            continue;
        }
        if(argv[i]==debugModeFlagOn){
            if(len(argv) > i+1 && argv[i+1] == password){
                i++;
                debugMode = 1;
                puts("Auth successful");
            }
            puts("Auth unsuccessful");
            continue;
        }
        if(debugMode==1){
            puts(argv[i]);
        }
    }
    printf("\n");
    return 0;
}