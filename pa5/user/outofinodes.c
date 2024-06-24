#include <stdio.h>
#include <time.h>
#include "../kernel/param.h"
#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "user.h"
#include "../kernel/fs.h"
#include "../kernel/fcntl.h"
#include "../kernel/syscall.h"
#include "../kernel/memlayout.h"
#include "../kernel/riscv.h"

void outofinodes(char *s)
{
    int nzz = 32*32;
    clock_t start, end;
    double cpu_time_used;
    
    // Measure time for the first loop
    start = clock();
    for(int i = 0; i < nzz; i++){
        char name[32];
        name[0] = 'z';
        name[1] = 'z';
        name[2] = '0' + (i / 32);
        name[3] = '0' + (i % 32);
        name[4] = '\0';
        unlink(name);
        int fd = open(name, O_CREATE|O_RDWR|O_TRUNC);
		printf("open %s\n", name);
        if(fd < 0){
            // failure is eventually expected.
            break;
        }
        close(fd);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken by first loop: %f seconds\n", cpu_time_used);

    // Measure time for the second loop
    start = clock();
    for(int i = 0; i < nzz; i++){
        char name[32];
        name[0] = 'z';
        name[1] = 'z';
        name[2] = '0' + (i / 32);
        name[3] = '0' + (i % 32);
        name[4] = '\0';
        unlink(name);
		printf("delete %s\n", name);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken by second loop: %f seconds\n", cpu_time_used);
}

int main()
{
    outofinodes("dummy");
    return 0;
}
