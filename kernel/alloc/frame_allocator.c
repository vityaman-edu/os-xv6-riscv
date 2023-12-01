/// Physical memory allocator, for user processes,
/// kernel stacks, page-table pages,
/// and pipe buffers. Allocates whole 4096-byte pages.

#include <kernel/alloc/buddy.h>
#include <kernel/alloc/frame_allocator.h>
#include <kernel/core/param.h>
#include <kernel/core/type.h>
#include <kernel/defs.h>
#include <kernel/hw/arch/riscv/register.h>
#include <kernel/hw/memlayout.h>
#include <kernel/sync/spinlock.h>

/// First address after kernel, defined by `kernel.ld`.
extern char end[];

void frame_allocator_init() {
  void* start = (char*)PGROUNDUP((uint64)end);
  void* end = (void*)PHYSTOP;
  buddy_init(start, end);
}

void frame_free(void* phys) {
  buddy_free(phys);
}

void* frame_allocate(void) {
  return buddy_malloc(PGSIZE);
}
