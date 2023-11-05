#ifndef XV6_KERNEL_PROCESS_FACTORY_H
#define XV6_KERNEL_PROCESS_FACTORY_H

#include "kernel/process/Process.h"
#include "kernel/process/Queue.h"

/// All system processes are stored here.
extern ProcessQueue ProcessFactory$Graveyard; // NOLINT: thread-safe

/// User should initialize this module before usage.
void ProcessFactory$Initialize();

/// Look in the `Graveyard` for an `UNUSED` proceses.
/// Or allocate memory using `Memory$Allocator`.
/// If found, initialize state required to run in
/// the kernel.
///
/// @return `Process` with `process->lock` held
/// @return `nullptr` if can't create more processes
Process* ProcessFactory$Create();

/// Return process back to `ProcessFactory`.
/// Caller must hold `process->lock`.
/// Any usage of the given `process` after
/// disposing.
///
/// @param `process` to dispose
void ProcessFactory$Dispose(Process* process);

#endif // XV6_KERNEL_PROCESS_FACTORY_H
