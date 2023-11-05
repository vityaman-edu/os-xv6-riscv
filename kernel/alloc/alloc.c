#include "kernel/alloc/buddy.h"

void* memory_allocate(uint64 nbytes) {
  return buddy_malloc(nbytes);
}

void memory_deallocate(void* addr) {
  buddy_free(addr);
}
