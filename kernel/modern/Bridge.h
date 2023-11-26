#pragma once

#include "kernel/legacy/types.h"

int UserVirtMemoryCopy(pagetable_t src, pagetable_t dst, UInt64 size);

void UserVirtMemoryDump(pagetable_t pagetable, UInt64 size);

int UVMCOWFixPageFault(pagetable_t pagetable, UInt64 size, UInt64 virt);

void GlobalFrameAllocatorInit();

void* GlobalFrameAllocatorAllocate();

void GlobalFrameAllocatorDeallocate(void* phys);

void GlobalFrameAllocatorReference(void* phys);
