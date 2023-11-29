// Mutual exclusion spin locks.

#include <kernel/core/param.h>
#include <kernel/core/type.h>
#include <kernel/defs.h>
#include <kernel/hardware/memlayout.h>
#include <kernel/hardware/riscv.h>
#include <kernel/process/proc.h>
#include <kernel/sync/spinlock.h>

void initlock(struct spinlock* lock, char* name) {
  lock->name = name;
  lock->locked = 0;
  lock->cpu = 0;
}

void acquire(struct spinlock* lock) {
  push_off(); // disable interrupts to avoid deadlock.
  if (holding(lock)) {
    panic("acquire");
  }

  // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
  //   a5 = 1
  //   s1 = &lk->locked
  //   amoswap.w.aq a5, a5, (s1)
  uint64 try = 0;
  const uint64 limit = 1000000;
  while (__sync_lock_test_and_set(&lock->locked, 1) != 0) {
    try += 1;
    if (try > limit) {
      try = 0;
      printf(
          "[warn] possible deadlock on '%s' acquired by %d\n",
          lock->name,
          lock->cpu->proc->pid
      );
    }
  }

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen strictly after the lock is acquired.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();

  // Record info about lock acquisition for holding() and debugging.
  lock->cpu = mycpu();
}

// Release the lock.
void release(struct spinlock* lock) {
  if (!holding(lock)) {
    panic("release");
  }

  lock->cpu = 0;

  // Tell the C compiler and the CPU to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released,
  // and that loads in the critical section occur strictly before
  // the lock is released.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code doesn't use a C assignment, since the C standard
  // implies that an assignment might be implemented with
  // multiple store instructions.
  // On RISC-V, sync_lock_release turns into an atomic swap:
  //   s1 = &lk->locked
  //   amoswap.w zero, zero, (s1)
  __sync_lock_release(&lock->locked);

  pop_off();
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int holding(struct spinlock* lock) {
  return (lock->locked && lock->cpu == mycpu());
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.

void push_off(void) {
  int old = intr_get();

  intr_off();
  if (mycpu()->noff == 0) {
    mycpu()->intena = old;
  }
  mycpu()->noff += 1;
}

void pop_off(void) {
  struct cpu* c = mycpu();
  if (intr_get()) {
    panic("pop_off - interruptible");
  }
  if (c->noff < 1) {
    panic("pop_off");
  }
  c->noff -= 1;
  if (c->noff == 0 && c->intena) {
    intr_on();
  }
}
