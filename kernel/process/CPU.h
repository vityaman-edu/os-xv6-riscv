#ifndef XV6_KERNEL_CPU_H
#define XV6_KERNEL_CPU_H

#include "kernel/process/proc.h"

typedef struct cpu CPU;

int CPU$Id();

CPU* CPU$Current();

#endif