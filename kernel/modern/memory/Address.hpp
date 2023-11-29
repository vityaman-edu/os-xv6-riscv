#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "MemoryTag.hpp"
#include <kernel/modern/library/error/Panic.hpp>

extern "C" {
#include <kernel/legacy/riscv.h>
}

namespace xv6::kernel::memory {

using library::error::Panic;

template <MemoryTag tag>
class Addr {
 public:
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

  static Addr unsafeFrom(std::uint64_t value) {
    if (MAXVA <= value) {
      Panic("Invalid Address");
    }
    return Addr(value);
  }

 private:
  explicit Addr(std::uint64_t value) : value_(value) {
  }

  std::uint64_t value_;
};

using Virt = Addr<MemoryTag::VIRTUAL>;
using Phys = Addr<MemoryTag::PHYSICAL>;

std::optional<Virt> VirtChecked(std::uint64_t value);

}  // namespace xv6::kernel::memory