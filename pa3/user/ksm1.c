//----------------------------------------------------------------
//
//  4190.307 Operating Systems (Spring 2024)
//
//  Project #4: KSM (Kernel Samepage Merging)
//
//  May 7, 2024
//
//  Jin-Soo Kim (jinsoo.kim@snu.ac.kr)
//  Systems Software & Architecture Laboratory
//  Dept. of Computer Science and Engineering
//  Seoul National University
//
//----------------------------------------------------------------

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define RDFD      0
#define WRFD      1

// KSMD sends a signal to process n
#define KSMD_SIGNAL(n)  write(kfds[n][WRFD], "x", 1)
// KSMD waits for a signal from process n
#define KSMD_WAIT(n)    read(pfds[n][RDFD], &m, 1)

// Process n sends a signal to KSMD
#define P_SIGNAL(n)     write(pfds[n][WRFD], "x", 1)
// Process n waits for a signal from KSMD
#define P_WAIT(n)       read(kfds[n][RDFD], &m, 1)


int a[4000];      // &a[0] == 0x1010;
                  
void
do_ksm()
{
  int scanned, merged, freemem;

  freemem = ksm(&scanned, &merged);
  printf("ksm: scanned=%d, merged=%d, freemem=%d\n",
        scanned, merged, freemem);
}

void
main(int argc, char *argv[])
{
  int kfds[3][2], pfds[3][2], pids[3];
  char m;

  for (int i = 0; i < 3; i++)
  {
    if (pipe(kfds[i]) != 0)
      exit(1);
    if (pipe(pfds[i]) != 0)
      exit(1);
  }

  // printf("pid %d: ksmd starts\n", getpid());
  do_ksm();
  // printf("stack : %p\n", pids);

  if ((pids[0] = fork()) == 0)
  {
    // child 0
    P_SIGNAL(0);
    P_WAIT(0);
    exit(0);
  }
  else
  {
    KSMD_WAIT(0);
    printf("KSMD: after forking child %d\n", pids[0]);
    do_ksm();

    if ((pids[1] = fork()) == 0)
    {
      // child 1
      P_SIGNAL(1);
      P_WAIT(1);
      exit(1);
    }
    else 
    {
      KSMD_WAIT(1);
      printf("KSMD: after forking child %d\n", pids[1]);
      do_ksm();

      if ((pids[2] = fork()) == 0)
      {
        // child 2
        P_SIGNAL(2);
        P_WAIT(2);
        exit(2);
      }
      else
      {
        KSMD_WAIT(2);
        printf("KSMD: after forking child %d\n", pids[2]);
        do_ksm();

        KSMD_SIGNAL(1);
        wait(0);
        printf("KSMD: after terminating child %d\n", pids[1]);
        do_ksm();

        KSMD_SIGNAL(0);
        wait(0);
        printf("KSMD: after terminating child %d\n", pids[0]);
        do_ksm();

        KSMD_SIGNAL(2);
        wait(0);
        printf("KSMD: after terminating child %d\n", pids[2]);
        do_ksm();

        exit(0);
      }
    }
  }
}
