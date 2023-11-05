#include "kernel/process/ProcessFactory.h"
#include "kernel/alloc/alloc.h"
#include "kernel/core/type.h"
#include "kernel/defs.h"
#include "kernel/hardware/memlayout.h"
#include "kernel/hardware/riscv.h"
#include "kernel/lib/Sequence.h"
#include "kernel/process/Process.h"
#include "kernel/process/Queue.h"
#include "kernel/sync/spinlock.h"

/// Kernel pagetable
extern pagetable_t kernel_pagetable; // NOLINT: no other way

/// Used to map kernel stack to newly
/// allocated process
static Sequence Process$Index; // NOLINT: thread-safe

/// Process Limit
static const Sequence$Number Process$Index$Max = 100;

/// Next Process ID (pid) sequence.
static Sequence Process$Id; // NOLINT: thread-safe

/// All processes are here.
ProcessQueue ProcessFactory$Graveyard; // NOLINT: thread-safe

extern void forkret(void);

/// Private functions
static void ProcessFactory$Reset(Process* process);
static void ProcessFactory$New(Process* process);
static bool ProcessFactory$Prepare(Process* process);
static Process* ProcessFactory$FindUnused();
static Process* ProcessFactory$Allocate();

void ProcessFactory$Initialize() {
  Sequence$New(&Process$Index);
  Sequence$New(&Process$Id);
  ProcessQueue$New(&ProcessFactory$Graveyard);
}

Process* ProcessFactory$Create() {
  Process* process = ProcessFactory$FindUnused();
  if (process == nullptr) {
    process = ProcessFactory$Allocate();
  }
  if (process == nullptr) {
    return nullptr;
  }

  if (!ProcessFactory$Prepare(process)) {
    ProcessFactory$Dispose(process);
    return nullptr;
  }

  return process;
}

void ProcessFactory$Dispose(Process* process) {
  ProcessFactory$Reset(process);
  release(&process->lock);
}

/// free a proc structure and the data hanging from it,
/// including user pages. p->lock must be held.
static void ProcessFactory$Reset(Process* process) {
  if (process->trapframe) {
    kfree((void*)process->trapframe);
  }
  process->trapframe = nullptr;

  if (process->pagetable) {
    proc_freepagetable(process->pagetable, process->sz);
  }
  process->pagetable = nullptr;

  process->sz = 0;
  process->pid = 0;
  process->parent = nullptr;
  process->name[0] = 0;
  process->chan = nullptr;
  process->killed = false;
  process->xstate = 0;
  process->state = UNUSED;
}

static void ProcessFactory$New(Process* process) {
  { // Reset all fields to zero
    process->state = UNUSED;
    process->chan = nullptr;
    process->killed = false;
    process->xstate = 0;
    process->pid = 0;
    process->parent = nullptr;
    process->kstack = nullptr;
    process->sz = 0;
    process->pagetable = nullptr;
    process->trapframe = nullptr;
    for (int i = 0; i < NOFILE; ++i) {
      process->ofile[i] = nullptr;
    }
    process->cwd = nullptr;
    process->name[0] = '\0';
  }

  { // Prepare process lock for usage
    initlock(&process->lock, "Process$Lock");
  }

  { // Map Kernel stack
    Sequence$Number index = Sequence$Next(&Process$Index);
    process->kstack = KSTACK((int)(index));
    char* phys = kalloc();
    if (phys == nullptr) {
      panic("kalloc returned nullptr");
    }
    uint64 virt = process->kstack;
    kvmmap(kernel_pagetable, virt, (uint64)phys, PGSIZE, PTE_R | PTE_W);
  }
}

static bool ProcessFactory$Prepare(Process* process) {
  process->pid = Sequence$Next(&Process$Id);
  process->state = USED;

  // Allocate a trapframe page.
  process->trapframe = kalloc();
  if (process->trapframe == nullptr) {
    return false;
  }

  // An empty user page table.
  process->pagetable = proc_pagetable(process);
  if (process->pagetable == nullptr) {
    return false;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&process->context, 0, sizeof(process->context));
  process->context.ra = (uint64)forkret;
  process->context.sp = process->kstack + PGSIZE;

  return true;
}

static Process* ProcessFactory$FindUnused() {
  ProcessQueue$ForEach(&ProcessFactory$Graveyard) {
    Process* process = ProcessQueue$Iterator$Process(it);
    acquire(&process->lock);
    if (process->state == UNUSED) {
      return process;
    }
    release(&process->lock);
  }
  return nullptr;
}

static Process* ProcessFactory$Allocate() {
  if (Sequence$Last(&Process$Index) > Process$Index$Max) {
    return nullptr;
  }

  ProcessQueue$Node* node = Memory$Allocate(sizeof(ProcessQueue$Node));
  if (node == nullptr) {
    return nullptr;
  }

  Process* process = &node->process;

  ProcessFactory$New(process);
  acquire(&process->lock);

  ProcessQueue$Push(&ProcessFactory$Graveyard, node);

  return process;
}
