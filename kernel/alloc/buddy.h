#pragma once

/// Buddy Allocator

#include <kernel/core/type.h>

/// Initialize the buddy allocator:
/// it manages memory from [base, end).
void buddy_init(void* base, void* end);

/// allocate nbytes, but malloc won't return anything
/// smaller than LEAF_SIZE
void* buddy_malloc(uint64 nbytes);

/// Free memory pointed to by p, which was earlier
/// allocated using bd_malloc.
void buddy_free(void* addr);