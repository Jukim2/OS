#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// extern struct proc proc[NPROC];

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// #ifdef SNU
/* Do not touch sys_time() */
uint64 
sys_time(void)
{
  uint64 x;

  asm volatile("rdtime %0" : "=r" (x));
  return x;
}
/* Do not touch sys_time() */

uint64
sys_sched_setattr(void)
{
  // FILL HERE
  struct proc *this_proc;
  int pid, runtime, period;

  argint(0, &pid);
  argint(1, &runtime);
  argint(2, &period);

  if (runtime < 1 || period < 1 || runtime > period)
    return (-1);

  if (pid == 0)
    this_proc = myproc();
  else
  {
    for(this_proc = proc; this_proc < &proc[NPROC]; this_proc++)
    {
      // acquire(&this_proc->lock);
      if (this_proc->pid == pid)
      {
        // release(&this_proc->lock);
        break;
      }
      // release(&this_proc->lock);
    }
  }

  if (this_proc == &proc[NPROC])
    return (-1);

  // acquire(&this_proc->lock);
  this_proc->deadline = ticks + period;
  this_proc->runtime = runtime;
  this_proc->period = period;
  this_proc->endtime = ticks;
  this_proc->state = RUNNABLE;
  // release(&this_proc->lock);
  return 0;
}

uint64
sys_sched_yield(void)
{
  // MODIFY THIS
  struct proc *this_proc = myproc();

  acquire(&this_proc->lock);
  this_proc->deadline += this_proc->period;
  this_proc->endtime = ticks;
  release(&this_proc->lock);

  yield();

  return 0;
}
// #endif
