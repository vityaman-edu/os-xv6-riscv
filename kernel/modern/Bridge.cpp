#include "memory/Address.hpp"

extern "C" {
#include <kernel/modern/Bridge.h>
}

#include <kernel/modern/memory/allocator/FrameAllocator.hpp>
#include <kernel/modern/memory/virt/AddressSpace.hpp>
#include <kernel/modern/memory/allocator/FrameAllocator.hpp>

using xv6::kernel::memory::allocator::FrameAllocator;
using xv6::kernel::memory::allocator::GlobalFrameAllocator;
using xv6::kernel::memory::virt::AddressSpace;
using xv6::kernel::memory::Phys;
using xv6::kernel::memory::Frame;

int UserVirtMemoryCopy(
    pagetable_t src, pagetable_t dst, UInt64 size
) {
  auto source = AddressSpace(src, size);
  auto destination = AddressSpace(dst, size);
  return source.CopyTo(destination);
}

void UserVirtMemoryDump(pagetable_t pagetable, UInt64 size) {
  AddressSpace(pagetable, size).print();
}

void GlobalFrameAllocatorInit() {
  GlobalFrameAllocator
      = (FrameAllocator*)bd_malloc(sizeof(FrameAllocator));
  new (GlobalFrameAllocator) FrameAllocator();
}

void* GlobalFrameAllocatorAllocate() {
  if (const auto frame = GlobalFrameAllocator->Allocate()) {
    return frame->begin().ptr();
  }
  return nullptr;
}

void GlobalFrameAllocatorDeallocate(void* phys) {
  const auto frame = Frame(Phys((std::uint64_t)phys));
  GlobalFrameAllocator->Deallocate(frame);
}
