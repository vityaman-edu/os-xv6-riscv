#pragma once

#include "Address.hpp"
#include "MemoryTag.hpp"
#include <kernel/modern/library/error/Panic.hpp>

namespace xv6::kernel::memory {

using library::error::Panic;

template <MemoryTag tag>
class Frage {
 public:
  static constexpr std::size_t kSize = 4096;

  explicit Frage(Addr<tag> begin) : begin_(begin) {
    if (begin.toInt() % kSize != 0) {
      Panic("invalid frage start address");
    }
  }

  Addr<tag> begin() const {
    return begin_;
  }

  Addr<tag> end() const {
    return begin_ + kSize;
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