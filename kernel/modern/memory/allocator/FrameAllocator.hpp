#pragma once

#include <optional>

#include <kernel/modern/memory/Frage.hpp>

namespace xv6::kernel::memory::allocator {

class FrameAllocator {
 public:
  static auto Allocate() -> std::optional<Frame>;
  static auto Deallocate(Frame frame) -> void;
  static auto Reference(Frame frame) -> void;
};

}  // namespace xv6::kernel::memory::allocator