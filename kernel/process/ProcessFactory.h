#ifndef XV6_KERNEL_PROCESS_FACTORY_H
#define XV6_KERNEL_PROCESS_FACTORY_H

#include "kernel/process/Process.h"
#include "kernel/process/Queue.h"

/// Reuses abandoned processes
extern ProcessQueue ProcessFactory$Graveyard;

void ProcessFactory$Initialize();

/// free a proc structure and the data hanging from it,
/// including user pages. p->lock must be held.
void ProcessFactory$Reset(Process* process);

void ProcessFactory$New(Process* process);

bool ProcessFactory$Prepare(Process* process);

/// Look in the process table for an UNUSED proc.
/// If found, initialize state required to run in
/// the kernel, and return with p->lock held.
/// If there are no free procs, or a memory
/// allocation fails, return 0.
Process* ProcessFactory$Create();

void ProcessFactory$Dispose(Process* process);

#endif // XV6_KERNEL_PROCESS_FACTORY_H
