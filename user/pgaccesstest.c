#include "user/user.h"
#include "stdio.h"
#include "kernel/types.h"

int main () {
    int* arr = (int*) sbrk(4096 * 2);
    arr[0] = 42;
    arr[1024] = 84;

    uint64 mask;
    if (pgaccess(arr, 2, mask) < 0) {
        printf("pgaccess failed.\n");
        exit(1);
    }

    printf("Accessed pages bitmask: %lx \n", mask);
    exit(0);
}