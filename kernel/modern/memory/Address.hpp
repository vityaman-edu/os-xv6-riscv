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

  [[nodiscard]] auto ptr() const -> std::uint8_t* {
    return (std::uint8_t*)value_;
  }

  [[nodiscard]] auto toInt() const -> std::uint64_t {
    return value_;
  }

  auto operator<(const Addr& rhs) const -> bool {
    return value_ < rhs.value_;
  }

  auto operator+(std::size_t shift) const -> Addr {
    return Addr(value_ + shift);
  }

 private:
  std::uint64_t value_;
};

using Virt = Addr<MemoryTag::VIRTUAL>;

using Phys = Addr<MemoryTag::PHYSICAL>;

}  // namespace xv6::kernel::memory