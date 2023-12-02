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
#include <kernel/memory/vm.h>
#include <kernel/sync/spinlock.h>

#define FRAME_MAX_COUNT (PHYSTOP / PGSIZE)

typedef uint16 ref_count_t;

static struct {
  ref_count_t ref_count[FRAME_MAX_COUNT];
  struct spinlock lock;
} frame_info;

/// First address after kernel, defined by `kernel.ld`.
extern char end[];

void frame_allocator_init() {
  void* start = (char*)PGROUNDUP((uint64)end);
  void* end = (void*)PHYSTOP;
  buddy_init(start, end);

  initlock(&frame_info.lock, "frame_info");
  memset(frame_info.ref_count, 0, FRAME_MAX_COUNT);
}

frame frame_parse(void* ptr) {
  const uint64 addr = (uint64)ptr;
  if (addr % PGSIZE != 0) {
    panic("frame.ptr is not PGSIZE aligned");
  }
  if (addr < (uint64)end) {
    panic("frame.ptr is in the kernel memory");
  }
  if (addr >= PHYSTOP) {
    panic("frame.ptr is out of PHYSTOP");
  }
  return (frame){ptr};
}

size_t frame_index(frame frame) {
  return frame.addr / PGSIZE;
}

void frame_free(frame frame) {
  const size_t index = frame_index(frame);
  acquire(&frame_info.lock);
  if (frame_info.ref_count[index] == 0) {
    printf("frame allocator: phys = %p\n", frame.ptr);
    panic("frame allocator: tried to free not referenced address");
  }
  frame_info.ref_count[index] -= 1;
  if (frame_info.ref_count[index] == 0) {
    buddy_free(frame.ptr);
  }
  release(&frame_info.lock);
}

frame frame_allocate() {
  void* ptr = buddy_malloc(PGSIZE);
  if (ptr == nullptr) {
    return (frame){.ptr = nullptr};
  }
  const frame frame = frame_parse(ptr);
  frame_reference(frame);
  return frame;
}

void frame_reference(frame frame) {
  const size_t index = frame_index(frame);
  acquire(&frame_info.lock);
  frame_info.ref_count[index] += 1;
  release(&frame_info.lock);
}
