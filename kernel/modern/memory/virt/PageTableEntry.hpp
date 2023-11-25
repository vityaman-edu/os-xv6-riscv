#pragma once

#include "kernel/modern/memory/address/Address.hpp"

extern "C" {
#include <kernel/riscv.h>
#include <kernel/types.h>
}

namespace xv6::kernel::memory::virt {

class PTERef {
 public:
  explicit PTERef(pte_t* const ptr) : ptr_(ptr) {
  }

  auto IsValid() const -> bool {
    return ((*ptr_) & PTE_V) != 0;
  }

  auto Physical() const -> address::Phys {
    return address::Phys(PTE2PA(*ptr_));
  }

  auto Flags() const -> int {
    return PTE_FLAGS(*ptr_);
  }

  PTERef(const PTERef&) = default;
  PTERef& operator=(const PTERef&) = default;
  PTERef(PTERef&&) = default;
  PTERef& operator=(PTERef&&) = default;
  ~PTERef() = default;

 private:
  pte_t* ptr_;
};

}  // namespace xv6::kernel::memory::virt