#pragma once

namespace xv6::kernel::memory {

enum class MemoryTag {
  VIRTUAL,
  PHYSICAL,
};

} // namespace xv6::kernel::memory