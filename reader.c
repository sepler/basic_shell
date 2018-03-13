#include <stdio.h>

int main(int argc, char* argv[]) {
    printf("Printing from reader: ");
    int i;
    printf("args:%d\n", argc);
    for (i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}