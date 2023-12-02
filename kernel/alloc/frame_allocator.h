#pragma once

#include <kernel/core/type.h>

/// Represents a region of physical memory
/// with size `PGSIZE`. `ptr` is PGSIZE aligned.
/// `ptr` can be `nullptr` when allocation fails.
typedef union {
  void* ptr;     // a pointer to frame start
  uint64 addr;   // `ptr` as an integer
  bool is_valid; // is a nullptr? DO NOT COMPARE WITH `TRUE`
} frame;

/// Validates a memory address and creates
/// a frame.
frame frame_parse(void* ptr);

typedef uint16 ref_count_t;

/// Initializes kernel frame allocator subsystem.
void frame_allocator_init();

/// Allocate one 4096-byte page of physical memory.
/// Returns a pointer that the kernel can use.
/// Returns 0 if the memory cannot be allocated.
frame frame_allocate();

/// Free the page of physical memory pointed at by v,
/// which normally should have been returned by a
/// call to frame_allocate(). (The exception is when
/// initializing the allocator; see kinit above).
void frame_free(frame frame);

/// Increments a given frame ref-counter to prevent
/// deallocation.
void frame_reference(frame frame);

ref_count_t frame_ref_count(frame frame);