#include "kernel/alloc/buddy.h"
#include "kernel/alloc/alloc.h"

void* Memory$Allocate(uint64 nbytes) {
  return buddy_malloc(nbytes);
}

void Memory$Deallocate(void* addr) {
  buddy_free(addr);
}
