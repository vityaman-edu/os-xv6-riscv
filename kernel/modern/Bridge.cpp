
extern "C" {
#include <kernel/modern/Bridge.h>
}

#include <kernel/modern/memory/virt/PageTable.hpp>

using xv6::kernel::memory::virt::PageTable;

auto UserVityalMemoryCopy(
    pagetable_t src, pagetable_t dst, UInt64 size
) -> int {
  auto source = PageTable(src, size);
  auto destination = PageTable(dst, size);
  return source.CopyTo(destination);
}
