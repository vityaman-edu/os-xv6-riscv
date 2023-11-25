
extern "C" {
#include <kernel/modern/Bridge.h>
}

#include <kernel/modern/memory/virt/AddressSpace.hpp>

using xv6::kernel::memory::virt::AddressSpace;

auto UserVirtMemoryCopy(pagetable_t src, pagetable_t dst, UInt64 size)
    -> int {
  auto source = AddressSpace(src, size);
  auto destination = AddressSpace(dst, size);
  return source.CopyTo(destination);
}

auto UserVirtMemoryDump(pagetable_t pagetable, UInt64 size) -> void {
  AddressSpace(pagetable, size).Print();
}
