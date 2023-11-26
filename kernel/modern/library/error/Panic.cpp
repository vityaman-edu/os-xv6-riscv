#include "Panic.hpp"

extern "C" {
#include <kernel/legacy/defs.h>
}

namespace xv6::kernel::library::error {

void Panic(const char* message) {
  panic(message);
}

}  // namespace xv6::kernel::library::error