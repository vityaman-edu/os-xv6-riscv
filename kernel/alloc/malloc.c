#include <kernel/alloc/buddy.h>
#include <kernel/alloc/malloc.h>

void* malloc(uint64 size) {
  return buddy_malloc(size);
}

void free(void* ptr) {
  buddy_free(ptr);
}