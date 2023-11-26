#pragma once

#include <cstddef>
#include <cstdint>

#include "MemoryTag.hpp"

namespace xv6::kernel::memory {

template <MemoryTag tag>
class Addr {
 public:
  explicit Addr(std::uint64_t value) : value_(value) {
  }

  [[nodiscard]] std::uint8_t* ptr() const {
    return (std::uint8_t*)value_;
  }

  [[nodiscard]] std::uint64_t toInt() const {
    return value_;
  }

  bool operator<(const Addr& rhs) const {
    return value_ < rhs.value_;
  }

  Addr operator+(std::size_t shift) const {
    return Addr(value_ + shift);
  }

 private:
  std::uint64_t value_;
};

using Virt = Addr<MemoryTag::VIRTUAL>;

using Phys = Addr<MemoryTag::PHYSICAL>;

}  // namespace xv6::kernel::memory