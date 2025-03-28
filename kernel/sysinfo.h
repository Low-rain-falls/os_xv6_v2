#include "types.h"

struct sysinfo
{
    uint64 freemem; // the number of bytes of free memory
    uint64 nproc; // the number of active processes
    uint64 nopenfiles; //the number of open files
};
