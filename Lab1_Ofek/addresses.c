#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int addr5;
int addr6;

int foo()
{
    return -1;
}
void point_at(void *p);
void foo1();
char g = 'g';
void foo2();

void secondary(int x)
{
    int addr2;
    int addr3;
    char *yos = "ree";
    int *addr4 = (int *)(malloc(50));
	int iarray[2];
    float farray[2];
    double darray[2];
    char carray[2];
	int iarray2[] = {1,2,3,4};
    char carray2[] = {'a','b','c','d'};
    int* iarray2Ptr;
    char* carray2Ptr;
    
	printf("- &addr2: %p\n", &addr2);
    printf("- &addr3: %p\n", &addr3);
    printf("- foo: %p\n", &foo);
    printf("- &addr5: %p\n", &addr5);
	printf("Print distances:\n");
    point_at(&addr5);

    printf("Print more addresses:\n");
    printf("- &addr6: %p\n", &addr6);
    printf("- yos: %p\n", yos);
    printf("- gg: %p\n", &g);
    printf("- addr4: %p\n", addr4);
    printf("- &addr4: %p\n", &addr4);

    printf("- &foo1: %p\n", &foo1);
    printf("- &foo2: %p\n", &foo2);

    printf("Print another distance:\n");
    printf("- &foo2 - &foo1: %ld\n", (long)&foo2 - (long)&foo1);

   
    printf("Arrays Mem Layout (T1b):\n");
    printf("- iarray:   %p\n", (void*)iarray);
    printf("- iarray+1: %p\n", (void*)(iarray+1));
    printf("- farray:   %p\n", (void*)farray);
    printf("- farray+1: %p\n", (void*)(farray+1));
    printf("- darray:   %p\n", (void*)darray);
    printf("- darray+1: %p\n", (void*)(darray+1));
    printf("- carray:   %p\n", (void*)carray);
    printf("- carray+1: %p\n", (void*)(carray+1));

    printf("Pointers and arrays (T1d):\n");
    iarray2Ptr = iarray2;
    carray2Ptr = carray2;
    printf("- iarray2 via pointer:");
    for (int i = 0; i < 4; i++)
        printf(" %d", *(iarray2Ptr + i));
    printf("\n");
    printf("- carray2 via pointer:");
    for (int i = 0; i < 4; i++)
        printf(" %c", *(carray2Ptr + i));
    printf("\n");

    int *p;
    printf("- uninitialized p: %p\n", (void*)p);

    free(addr4);
}

int main(int argc, char **argv)
{ 

    printf("Print function argument addresses:\n");

    printf("- &argc %p\n", &argc);
    printf("- argv %p\n", argv);
    printf("- &argv %p\n", &argv);
	
	secondary(0);
    
    printf("Command line arg addresses (T1e):\n");
    printf("- argv:       %p\n", (void*)argv);
    for (int i = 0; i < argc; i++) {
        printf("- argv[%d]:    %p  ->  \"%s\"\n", i, (void*)argv[i], argv[i]);
    }

    return 0;
}

void point_at(void *p)
{
    int local;
    static int addr0 = 2;
    static int addr1;

    long dist1 = (size_t)&addr6 - (size_t)p;
    long dist2 = (size_t)&local - (size_t)p;
    long dist3 = (size_t)&foo - (size_t)p;

    printf("- dist1: (size_t)&addr6 - (size_t)p: %ld\n", dist1);
    printf("- dist2: (size_t)&local - (size_t)p: %ld\n", dist2);
    printf("- dist3: (size_t)&foo - (size_t)p:  %ld\n", dist3);
    
    printf("Check long type mem size (T1a):\n");
    printf("- sizeof(long): %zu\n", sizeof(long));

    printf("- addr0: %p\n", &addr0);
    printf("- addr1: %p\n", &addr1);
}

void foo1()
{
    printf("foo1\n");
}

void foo2()
{
    printf("foo2\n");
}