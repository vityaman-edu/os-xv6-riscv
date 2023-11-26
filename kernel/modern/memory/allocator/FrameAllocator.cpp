#include <optional>

#include <kernel/modern/memory/allocator/FrameAllocator.hpp>
#include <kernel/modern/library/error/Panic.hpp>

extern "C" {
#include <kernel/legacy/defs.h>
}

namespace xv6::kernel::memory::allocator {

auto FrameAllocator::Allocate() -> std::optional<Frame> {
  auto* const ptr = kalloc();
  if (ptr == nullptr) {
    return std::nullopt;
  }
  return Frame(Phys((std::uint64_t)ptr));
}

auto FrameAllocator::Deallocate(Frame frame) -> void {
  kfree(frame.begin().ptr());
}

auto FrameAllocator::Reference(Frame frame) -> void {
  (void)frame;
  library::error::Panic("Not implemented");
}

}  // namespace xv6::kernel::memory::allocator