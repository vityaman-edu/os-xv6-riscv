#include <optional>

#include <kernel/modern/memory/allocator/FrameAllocator.hpp>
#include <kernel/modern/library/error/Panic.hpp>
#include "FrameAllocator.hpp"

extern "C" {
#include <kernel/legacy/defs.h>
}

namespace xv6::kernel::memory::allocator {

FrameAllocator::FrameAllocator()
    : _info({0}), _lock("GlobalFrameAllocator") {
  printf("GlobalFrameAllocator initialized!\n");
  printf("  Max frames count: %d\n", (int)MAX_FRAMES_COUNT);
}

std::optional<Frame> FrameAllocator::Allocate() {
  auto guard = _lock.lock();

  auto* const ptr = bd_malloc(Frame::Size);
  if (ptr == nullptr) {
    return std::nullopt;
  }

  const auto frame = Frame(Phys::unsafeFrom((std::uint64_t)ptr));
  if (!_info[frame.index()].isFree()) {
    Panic("[error] Buddy allocator returned a referenced frame");
  }

  _info[frame.index()].references_count += 1;
  return frame;
}

void FrameAllocator::Deallocate(Frame frame) {
  auto guard = _lock.lock();

  if (FrameAllocator::MAX_FRAMES_COUNT <= frame.index()) {
    Panic("Tried to deallocate out of bounds");
  }

  auto& info = _info[frame.index()];
  if (info.isFree()) {
    Panic("Tried to deallocate a free frame");
  }

  info.references_count -= 1;

  if (!info.isFree()) {
    return;
  }

  bd_free(frame.begin().ptr());
}

void FrameAllocator::Reference(Frame frame) {
  auto guard = _lock.lock();

  if (FrameAllocator::MAX_FRAMES_COUNT <= frame.index()) {
    Panic("Tried to reference out of bounds");
  }

  auto& info = _info[frame.index()];
  if (info.isFree()) {
    Panic("Tried to reference a free frame");
  }

  info.references_count += 1;
}

std::size_t FrameAllocator::ReferencesCount(Frame frame) {
  auto guard = _lock.lock();

  if (FrameAllocator::MAX_FRAMES_COUNT <= frame.index()) {
    Panic("Tried to get references out of bounds");
  }

  const auto& info = _info[frame.index()];
  return info.references_count;
}

void FrameAllocator::Print() {

}

FrameAllocator* GlobalFrameAllocator = nullptr;

}  // namespace xv6::kernel::memory::allocator