#ifndef XV6_KERNEL_PROCESS_H
#define XV6_KERNEL_PROCESS_H

#include "kernel/process/proc.h"

/// Process data structure.
typedef struct proc Process;

/// @return the current `Process`, or `nullptr` if none
Process* Process$Current();

#endif // XV6_KERNEL_PROCESS_H
