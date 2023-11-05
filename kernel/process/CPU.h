#ifndef XV6_KERNEL_CPU_H
#define XV6_KERNEL_CPU_H

#include "kernel/process/proc.h"

typedef struct cpu CPU;

/// Must be called with interrupts disabled,
/// to prevent race with process being moved
/// to a different CPU.
int CPU$Id();

/// Return this CPU's cpu struct.
/// Interrupts must be disabled.
CPU* CPU$Current();

#endif