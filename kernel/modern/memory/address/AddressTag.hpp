#pragma once

namespace xv6::kernel::memory::address {

enum class AddressTag {
  VIRTUAL,
  PHYSICAL,
};

}  // namespace xv6::kernel::memory::address