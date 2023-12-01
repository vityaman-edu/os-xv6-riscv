#pragma once

void frame_allocator_init();

/// Allocate one 4096-byte page of physical memory.
/// Returns a pointer that the kernel can use.
/// Returns 0 if the memory cannot be allocated.
void* frame_allocate();

/// Free the page of physical memory pointed at by v,
/// which normally should have been returned by a
/// call to frame_allocate(). (The exception is when
/// initializing the allocator; see kinit above.)
void frame_free(void* phys);
