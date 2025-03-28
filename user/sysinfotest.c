#include "user/user.h"
#include "kernel/sysinfo.h"

void test_sysinfo() {
    struct sysinfo info;

    if (sysinfo(&info) < 0) {
        printf("sysinfo: system call failed\n");
        exit(0);
    }

    printf("Free memory: %ld bytes\n", info.freemem);
    printf("Number of active processes: %ld\n", info.nproc);
    printf("Number of open files: %ld\n", info.nopenfiles);
}

int main () {
    test_sysinfo();
    exit(0);
}