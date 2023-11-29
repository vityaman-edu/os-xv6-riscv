#pragma once

#include <cstddef>

namespace xv6::kernel::library::math {

template <class Int>
auto DigitsCount(Int number) -> std::size_t {
  std::size_t count = 1;
  number /= 10;
  while (number != 0) {
    count += 1;
    number /= 10;
  }
  return count;
}

}  // namespace xv6::kernel::library::math