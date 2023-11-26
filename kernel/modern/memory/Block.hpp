#pragma once

#include "MemoryTag.hpp"
#include "Address.hpp"
#include <kernel/modern/library/error/Panic.hpp>

namespace xv6::kernel::memory {

using library::error::Panic;

template <MemoryTag tag>
class Block {
 public:
  using Address = Addr<tag>;

  Block(Address begin, Address end) : begin_(begin), end_(end) {
    if (!(begin < end)) {
      Panic("Invalid block as begin >= end");
    }
  }

  Address begin() const noexcept {
    return begin_;
  }

  Address end() const noexcept {
    return end_;
  }

 private:
  Address begin_;
  Address end_;
};

}  // namespace xv6::kernel::memory