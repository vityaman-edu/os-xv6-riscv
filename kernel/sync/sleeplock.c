// Sleeping locks

#include <kernel/core/param.h>
#include <kernel/core/type.h>
#include <kernel/defs.h>
#include <kernel/hw/memlayout.h>
#include <kernel/hw/arch/riscv/register.h>
#include <kernel/process/proc.h>
#include <kernel/sync/sleeplock.h>
#include <kernel/sync/spinlock.h>

void initsleeplock(struct sleeplock* lock, char* name) {
  initlock(&lock->lk, "sleep lock");
  lock->name = name;
  lock->locked = 0;
  lock->pid = 0;
}

void acquiresleep(struct sleeplock* lock) {
  acquire(&lock->lk);
  while (lock->locked) {
    sleep(lock, &lock->lk);
  }
  lock->locked = 1;
  lock->pid = myproc()->pid;
  release(&lock->lk);
}

void releasesleep(struct sleeplock* lock) {
  acquire(&lock->lk);
  lock->locked = 0;
  lock->pid = 0;
  wakeup(lock);
  release(&lock->lk);
}

int holdingsleep(struct sleeplock* lock) {
  acquire(&lock->lk);
  int r = lock->locked && (lock->pid == myproc()->pid);
  release(&lock->lk);
  return r;
}
