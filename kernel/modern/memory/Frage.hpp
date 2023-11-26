#pragma once

#include "Address.hpp"
#include "MemoryTag.hpp"
#include <kernel/modern/library/error/Panic.hpp>

extern "C" {
#include <kernel/legacy/memlayout.h>
}

namespace xv6::kernel::memory {

using library::error::Panic;

template <MemoryTag tag>
class Frage {
 public:
  static constexpr std::size_t Size = 4096;

  explicit Frage(Addr<tag> begin) : begin_(begin) {
    if (begin.toInt() % Size != 0) {
      Panic("invalid frage start address");
    }
  }

  Addr<tag> begin() const {
    return begin_;
  }

  Addr<tag> end() const {
    return begin_ + Size;
  }

  [[nodiscard]] std::size_t index() const {
    return (begin_.toInt() - KERNBASE) / Size;
  }

  static Frage containing(Addr<tag> addr) {
    return Frage(Addr<tag>(PGROUNDDOWN(addr.toInt())));
  }

 private:
  Addr<tag> begin_;
};

using Frame = Frage<MemoryTag::PHYSICAL>;

using Page = Frage<MemoryTag::VIRTUAL>;

}  // namespace xv6::kernel::memory