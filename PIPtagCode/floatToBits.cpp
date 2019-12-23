#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    float a = strtof(argv[1],NULL);

    printf("%X\n", *(int *)&a);

    return 0;
}
