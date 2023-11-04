#ifndef XV6_KERNEL_BUDDY_H
#define XV6_KERNEL_BUDDY_H

/// Buddy Allocator

#include "types.h"

/// Initialize the buddy allocator:
/// it manages memory from [base, end).
void bd_init(void* base, void* end);

/// allocate nbytes, but malloc won't return anything
/// smaller than LEAF_SIZE
void* bd_malloc(uint64 nbytes);

/// Free memory pointed to by p, which was earlier
/// allocated using bd_malloc.
void bd_free(void* addr);

#endif // XV6_KERNEL_BUDDY_H