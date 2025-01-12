// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "kernel/core/type.h"
#include "kernel/core/param.h"
#include "kernel/hardware/memlayout.h"
#include "kernel/sync/spinlock.h"
#include "kernel/hardware/riscv.h"
#include "kernel/defs.h"

#include "buddy.h"

/// First address after kernel, defined by `kernel.ld`.
extern char end[];

void kinit() {
  char* p = (char*)PGROUNDUP((uint64)end);
  buddy_init(p, (void*)PHYSTOP);
}

/// Free the page of physical memory pointed at by v,
/// which normally should have been returned by a
/// call to kalloc().  (The exception is when
/// initializing the allocator; see kinit above.)
void kfree(void* pa) {
  buddy_free(pa);
}

/// Allocate one 4096-byte page of physical memory.
/// Returns a pointer that the kernel can use.
/// Returns 0 if the memory cannot be allocated.
void* kalloc(void) {
  return buddy_malloc(PGSIZE);
}
