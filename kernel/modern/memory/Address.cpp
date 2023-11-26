#include "Address.hpp"

namespace xv6::kernel::memory {

std::optional<Virt> VirtChecked(std::uint64_t value) {
  if (MAXVA <= value) {
    return std::nullopt;
  }
  return Virt::unsafeFrom(value);
}

} // namespace xv6::kernel::memory