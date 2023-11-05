#ifndef XV6_KERNEL_ALLOC_H
#define XV6_KERNEL_ALLOC_H

#include "kernel/core/type.h"

void* memory_allocate(uint64 nbytes);

void memory_deallocate(void* addr);

#endif // XV6_KERNEL_ALLOC_H