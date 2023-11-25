#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

UInt64 sys_exit(void) {
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

UInt64 sys_getpid(void) { return myproc()->pid; }

UInt64 sys_fork(void) { return fork(); }

UInt64 sys_wait(void) {
  UInt64 p;
  argaddr(0, &p);
  return wait(p);
}

UInt64 sys_sbrk(void) {
  UInt64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

UInt64 sys_sleep(void) {
  int n;
  UInt32 ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while ((int)(ticks - ticks0) < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

UInt64 sys_kill(void) {
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
UInt64 sys_uptime(void) {
  UInt32 xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

UInt64 sys_dump(void) { return dump(); }

UInt64 sys_dump2(void) {
  int pid = 0;
  int register_num = 0; 
  UInt64 return_value = 0;

  argint(0, &pid);
  argint(1, &register_num);
  argaddr(2, &return_value);

  return dump2(pid, register_num, return_value);
}