#pragma once

#include <optional>

#include <kernel/modern/memory/Frage.hpp>

namespace xv6::kernel::memory::allocator {

class FrameAllocator {
 public:
  static std::optional<Frame> Allocate();
  static void Deallocate(Frame frame);
  static void Reference(Frame frame);
};

}  // namespace xv6::kernel::memory::allocator