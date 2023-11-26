#include <optional>

#include <kernel/modern/memory/allocator/FrameAllocator.hpp>
#include <kernel/modern/library/error/Panic.hpp>

extern "C" {
#include <kernel/legacy/defs.h>
}

namespace xv6::kernel::memory::allocator {

FrameAllocator::FrameAllocator()
    : _info(), _lock("GlobalFrameAllocator") {
  printf("GlobalFrameAllocator initialized!\n");
}

std::optional<Frame> FrameAllocator::Allocate() {
  auto guard = _lock.lock();

  auto* const ptr = bd_malloc(Frame::Size);
  if (ptr == nullptr) {
    return std::nullopt;
  }

  const auto frame = Frame(Phys((std::uint64_t)ptr));
  _info[frame.index()].references_count += 1;
  return frame;
}

void FrameAllocator::Deallocate(Frame frame) {
  auto guard = _lock.lock();

  auto& info = _info[frame.index()];
  if (info.isFree()) {
    Panic("Tried to deallocate free frame");
  }

  info.references_count -= 1;
  if (!info.isFree()) {
    return;
  }

  bd_free(frame.begin().ptr());
}

void FrameAllocator::Reference(Frame frame) {
  auto guard = _lock.lock();

  (void)frame;
  library::error::Panic("Not implemented");
}

FrameAllocator* GlobalFrameAllocator = nullptr;

}  // namespace xv6::kernel::memory::allocator