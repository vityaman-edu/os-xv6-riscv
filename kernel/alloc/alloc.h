#ifndef XV6_KERNEL_ALLOC_H
#define XV6_KERNEL_ALLOC_H

#include "kernel/core/type.h"

void* Memory$Allocate(uint64 nbytes);

void Memory$Deallocate(void* addr);

#endif // XV6_KERNEL_ALLOC_H