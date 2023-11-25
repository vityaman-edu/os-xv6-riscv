#include "kernel/types.h"

// Saved registers for kernel context switches.
struct context {
  UInt64 ra;
  UInt64 sp;

  // callee-saved
  UInt64 s0;
  UInt64 s1;
  UInt64 s2;
  UInt64 s3;
  UInt64 s4;
  UInt64 s5;
  UInt64 s6;
  UInt64 s7;
  UInt64 s8;
  UInt64 s9;
  UInt64 s10;
  UInt64 s11;
};

// Per-CPU state.
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ UInt64 kernel_satp;   // kernel page table
  /*   8 */ UInt64 kernel_sp;     // top of process's kernel stack
  /*  16 */ UInt64 kernel_trap;   // usertrap()
  /*  24 */ UInt64 epc;           // saved user program counter
  /*  32 */ UInt64 kernel_hartid; // saved kernel tp
  /*  40 */ UInt64 ra;
  /*  48 */ UInt64 sp;
  /*  56 */ UInt64 gp;
  /*  64 */ UInt64 tp;
  /*  72 */ UInt64 t0;
  /*  80 */ UInt64 t1;
  /*  88 */ UInt64 t2;
  /*  96 */ UInt64 s0;
  /* 104 */ UInt64 s1;
  /* 112 */ UInt64 a0;
  /* 120 */ UInt64 a1;
  /* 128 */ UInt64 a2;
  /* 136 */ UInt64 a3;
  /* 144 */ UInt64 a4;
  /* 152 */ UInt64 a5;
  /* 160 */ UInt64 a6;
  /* 168 */ UInt64 a7;
  /* 176 */ UInt64 s2;
  /* 184 */ UInt64 s3;
  /* 192 */ UInt64 s4;
  /* 200 */ UInt64 s5;
  /* 208 */ UInt64 s6;
  /* 216 */ UInt64 s7;
  /* 224 */ UInt64 s8;
  /* 232 */ UInt64 s9;
  /* 240 */ UInt64 s10;
  /* 248 */ UInt64 s11;
  /* 256 */ UInt64 t3;
  /* 264 */ UInt64 t4;
  /* 272 */ UInt64 t5;
  /* 280 */ UInt64 t6;
};

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  struct proc* next;
  struct proc* prev;

  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // wait_lock must be held when using this:
  struct proc *parent;         // Parent process

  // these are private to the process, so p->lock need not be held.
  UInt64 kstack;               // Virtual address of kernel stack
  UInt64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};
