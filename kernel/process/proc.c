#include "kernel/core/type.h"
#include "kernel/core/param.h"
#include "kernel/hardware/memlayout.h"
#include "kernel/hardware/riscv.h"
#include "kernel/lib/list.h"
#include "kernel/process/Process.h"
#include "kernel/process/ProcessFactory.h"
#include "kernel/process/CPU.h"
#include "kernel/process/Queue.h"
#include "kernel/sync/spinlock.h"
#include "kernel/defs.h"

struct proc* initproc;

extern void forkret(void);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// initialize the proc table.
void procinit(void) {
  initlock(&wait_lock, "wait_lock");
  ProcessFactory$Initialize();
}

int cpuid() {
  return CPU$Id();
}

struct cpu* mycpu(void) {
  return CPU$Current();
}

struct proc* myproc(void) {
  return Process$Current();
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t proc_pagetable(struct proc* p) {
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X)
      < 0) {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if (mappages(
          pagetable, TRAPFRAME, PGSIZE, (uint64)(p->trapframe), PTE_R | PTE_W
      )
      < 0) {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 size) {
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, size);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = { // NOLINT
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02, 0x97, 0x05, 0x00, // NOLINT
    0x00, 0x93, 0x85, 0x35, 0x02, 0x93, 0x08, 0x70, 0x00, 0x73, 0x00, // NOLINT
    0x00, 0x00, 0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00, 0xef, // NOLINT
    0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69, 0x74, 0x00, 0x00, 0x24, // NOLINT
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                   // NOLINT
};

// Set up first user process.
void userinit(void) {
  struct proc* p = ProcessFactory$Create();
  initproc = p;

  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->memory_size = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;     // user program counter
  p->trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n) {
  struct proc* p = myproc();

  uint64 sz = p->memory_size;
  if (n > 0) {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if (n < 0) {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->memory_size = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void) {
  struct proc* p = myproc();

  struct proc* np; // Allocate process.
  if ((np = ProcessFactory$Create()) == 0) {
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->memory_size) < 0) {
    // printf("[WARN] fork failed\n");
    ProcessFactory$Dispose(np);
    return -1;
  }
  np->memory_size = p->memory_size;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for (int i = 0; i < NOFILE; i++) {
    if (p->ofile[i]) {
      np->ofile[i] = filedup(p->ofile[i]);
    }
  }
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  int pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc* p) {
  ProcessQueue$ForEach(&ProcessFactory$Graveyard) {
    Process* pp = ProcessQueue$Iterator$Process(it);
    if (pp->parent == p) {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status) {
  struct proc* process = myproc();

  if (process == initproc) {
    panic("init exiting");
  }

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++) {
    if (process->ofile[fd]) {
      struct file* file = process->ofile[fd];
      fileclose(file);
      process->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(process->cwd);
  end_op();
  process->cwd = nullptr;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(process);

  // Parent might be sleeping in wait().
  wakeup(process->parent);

  acquire(&process->lock);

  process->xstate = status;
  process->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr) {
  struct proc* p = myproc();

  acquire(&wait_lock);

  for (;;) {
    // Scan through table looking for exited children.
    bool havekids = false;

    ProcessQueue$ForEach(&ProcessFactory$Graveyard) {
      Process* pp = ProcessQueue$Iterator$Process(it);

      if (pp->parent == p) {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = true;
        if (pp->state == ZOMBIE) {
          // Found one.
          int pid = pp->pid;
          if (addr != 0
              && copyout(
                     p->pagetable, addr, (char*)&pp->xstate, sizeof(pp->xstate)
                 ) < 0) {
            // printf("[INFO] copyout failed\n");
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }

          // printf("[INFO] found zombie kid\n");
          ProcessFactory$Dispose(pp);
          release(&wait_lock);
          return pid;
        }

        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p)) {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void) {
  struct cpu* c = mycpu();

  c->proc = 0;
  for (;;) {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    ProcessQueue$ForEach(&ProcessFactory$Graveyard) {
      Process* p = ProcessQueue$Iterator$Process(it);

      acquire(&p->lock);
      if (p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void) {
  struct proc* p = myproc();

  if (!holding(&p->lock)) {
    panic("sched p->lock");
  }
  if (mycpu()->noff != 1) {
    panic("sched locks");
  }
  if (p->state == RUNNING) {
    panic("sched running");
  }
  if (intr_get()) {
    panic("sched interruptible");
  }

  int intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void) {
  struct proc* p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void) {
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void* chan, struct spinlock* lk) {
  struct proc* p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); // DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void* chan) {
  ProcessQueue$ForEach(&ProcessFactory$Graveyard) {
    Process* p = ProcessQueue$Iterator$Process(it);

    if (p != myproc()) {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid) {
  ProcessQueue$ForEach(&ProcessFactory$Graveyard) {
    Process* p = ProcessQueue$Iterator$Process(it);

    acquire(&p->lock);
    if (p->pid == pid) {
      p->killed = 1;
      if (p->state == SLEEPING) {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void setkilled(struct proc* p) {
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int killed(struct proc* p) {
  acquire(&p->lock);
  int is_killed = p->killed;
  release(&p->lock);
  return is_killed;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void* src, uint64 len) {
  struct proc* p = myproc();

  if (user_dst) {
    return copyout(p->pagetable, dst, src, len);
  }

  memmove((char*)dst, src, len);
  return 0;
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void* dst, int user_src, uint64 src, uint64 len) {
  struct proc* p = myproc();

  if (user_src) {
    return copyin(p->pagetable, dst, src, len);
  }

  memmove(dst, (char*)src, len);
  return 0;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void) {
  static char* states[] = {
      [UNUSED] = "unused",
      [USED] = "used",
      [SLEEPING] = "sleep ",
      [RUNNABLE] = "runble",
      [RUNNING] = "run   ",
      [ZOMBIE] = "zombie",
  };

  printf("\n");
  ProcessQueue$ForEach(&ProcessFactory$Graveyard) {
    Process* p = ProcessQueue$Iterator$Process(it);

    if (p->state == UNUSED) {
      continue;
    }

    char* state = nullptr;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state]) {
      state = states[p->state];
    } else {
      state = "???";
    }

    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

static uint64 proc_register_s_value_at(const struct proc* this, int n) {
  switch (n) {
  case 0: // NOLINT
    return this->trapframe->s0;
  case 1: // NOLINT
    return this->trapframe->s1;
  case 2: // NOLINT
    return this->trapframe->s2;
  case 3: // NOLINT
    return this->trapframe->s3;
  case 4: // NOLINT
    return this->trapframe->s4;
  case 5: // NOLINT
    return this->trapframe->s5;
  case 6: // NOLINT
    return this->trapframe->s6;
  case 7: // NOLINT
    return this->trapframe->s7;
  case 8: // NOLINT
    return this->trapframe->s8;
  case 9: // NOLINT
    return this->trapframe->s9;
  case 10: // NOLINT
    return this->trapframe->s10;
  case 11: // NOLINT
    return this->trapframe->s11;
  default:
    panic("registers s are numbered in range 0-11");
  }
}

int dump() {
  const struct proc* proc = myproc();

  const uint64 mask = 0x00000000FFFFFFFF;
  for (int i = 2; i <= 11; ++i) { // NOLINT
    const uint64 value = proc_register_s_value_at(proc, i);
    const int display = (int)(value & mask);
    printf("s%d\t= %d (%x)\n", i, display, display);
  }

  return 0;
}

/// Acquires found process lock, so do not forget to
/// call `release` after usage.
/// returns `NULL` when not found.
static struct proc* proc_acquire_by_id(int pid) {
  ProcessQueue$ForEach(&ProcessFactory$Graveyard) {
    Process* process = ProcessQueue$Iterator$Process(it);

    acquire(&process->lock);
    if (process->pid == pid && !process->killed) {
      return process;
    }
    release(&process->lock);
  }
  return 0;
}

/// Checks that `this` is a child process of `parent`.
/// Caller should hold `parent` and `this` locks.
static int proc_is_child(const struct proc* this, const struct proc* parent) {
  for (const struct proc* process = this; //
       process != 0;
       process = process->parent) {
    if (process->pid == parent->pid) {
      return 1;
    }
  }
  return 0;
}

/// Read register `s{reg_num}` to address `return_value`
///
/// @param pid process id to read register
/// @param reg_num in range [2, 11]
/// @param ret_addr where to store result
int dump2(int pid, int reg_num, uint64 ret_addr) {
  struct proc* caller = myproc();
  struct proc* target = proc_acquire_by_id(pid);

  if (!target) {
    return -2;
  }

  if (!proc_is_child(target, caller)) {
    release(&target->lock);
    return -1;
  }

  if (!(2 <= reg_num && reg_num <= 11)) { // NOLINT
    release(&target->lock);
    return -3;
  }

  const uint64 value = proc_register_s_value_at(target, reg_num);

  release(&target->lock);

  const int user_dst = 1;
  if (either_copyout(user_dst, ret_addr, (char*)(&value), sizeof(value)) < 0) {
    return -4;
  }

  return 0;
}
