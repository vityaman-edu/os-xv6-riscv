#pragma once

#include <optional>
#include <array>

#include <kernel/modern/memory/Frage.hpp>
#include <kernel/modern/library/sync/Spinlock.hpp>

extern "C" {
#include <kernel/legacy/memlayout.h>
}

namespace xv6::kernel::memory::allocator {

using library::sync::Spinlock;

class FrameAllocator {
 private:
  static constexpr std::size_t MAX_FRAMES_COUNT
      = PHYSTOP / Frame::Size + 1;

  struct FrameInfo {
    std::size_t references_count = 0;

    bool isFree() const {
      return references_count == 0;
    }
  };

 public:
  FrameAllocator();

  std::optional<Frame> Allocate();
  void Deallocate(Frame frame);
  void Reference(Frame frame);

 private:
  std::array<FrameInfo, MAX_FRAMES_COUNT> _info;
  Spinlock _lock;
};

extern FrameAllocator* GlobalFrameAllocator;

}  // namespace xv6::kernel::memory::allocator