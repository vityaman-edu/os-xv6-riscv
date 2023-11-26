#include <optional>

#include <kernel/modern/memory/allocator/FrameAllocator.hpp>
#include <kernel/modern/library/error/Panic.hpp>

extern "C" {
#include <kernel/legacy/defs.h>
}

namespace xv6::kernel::memory::allocator {

std::optional<Frame> FrameAllocator::Allocate() {
  auto* const ptr = kalloc();
  if (ptr == nullptr) {
    return std::nullopt;
  }
  return Frame(Phys((std::uint64_t)ptr));
}

void FrameAllocator::Deallocate(Frame frame) {
  kfree(frame.begin().ptr());
}

void FrameAllocator::Reference(Frame frame) {
  (void)frame;
  library::error::Panic("Not implemented");
}

}  // namespace xv6::kernel::memory::allocator