#pragma once

#include "MemoryTag.hpp"
#include "Address.hpp"
#include <kernel/modern/library/error/Panic.hpp>

namespace xv6::kernel::memory {

using library::error::Panic;

template <class Unit>
class Block {
 public:
  Block(Unit begin, Unit end) : begin_(begin), end_(end) {
    if (!(begin < end)) {
      Panic("Invalid block as begin >= end");
    }
  }

  Unit begin() const noexcept {
    return begin_;
  }

  Unit end() const noexcept {
    return end_;
  }

 private:
  Unit begin_;
  Unit end_;
};

}  // namespace xv6::kernel::memory