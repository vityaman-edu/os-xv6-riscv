#pragma once

#include <cstdint>

#include "AddressTag.hpp"

namespace xv6::kernel::memory::address {

template <AddressTag tag>
class Address {
 public:
  explicit Address(std::uint64_t const value) : value_(value) {}

  [[nodiscard]] auto AsPtr() const -> std::uint8_t* {
    return (std::uint8_t*)value_;
  }

  [[nodiscard]] auto ToUInt64() const -> std::uint64_t {
    return value_;
  }

 private:
  std::uint64_t value_;
};

using Virt = Address<AddressTag::VIRTUAL>;

using Phys = Address<AddressTag::PHYSICAL>;

}  // namespace xv6::kernel::memory::address