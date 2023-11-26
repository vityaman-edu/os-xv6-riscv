#pragma once

#include "kernel/modern/memory/Address.hpp"

extern "C" {
#include <kernel/riscv.h>
#include <kernel/defs.h>
#include <kernel/types.h>
}

namespace xv6::kernel::memory::virt {

class PageTableEntry {
 public:
  explicit PageTableEntry(pte_t* const ptr) : ptr_(ptr) {
  }

  auto isValid() const -> bool {
    return ((*ptr_) & PTE_V) != 0;
  }

  auto isReadable() const -> bool {
    return ((*ptr_) & PTE_R) != 0;
  }

  auto isWrittable() const -> bool {
    return ((*ptr_) & PTE_W) != 0;
  }

  auto isExecutable() const -> bool {
    return ((*ptr_) & PTE_X) != 0;
  }

  auto isUserAccesible() const -> bool {
    return ((*ptr_) & PTE_U) != 0;
  }

  auto physical() const -> Phys {
    return Phys(PTE2PA(*ptr_));
  }

  auto frame() const -> Frame {
    return Frame(physical());
  }

  auto flags() const -> int {
    return PTE_FLAGS(*ptr_);
  }

  auto print() const -> void {
    const auto* v = isValid() ? "V" : " ";
    const auto* r = isReadable() ? "R" : " ";
    const auto* w = isWrittable() ? "W" : " ";
    const auto* e = isExecutable() ? "X" : " ";
    const auto* u = isUserAccesible() ? "U" : " ";
    const auto p = physical().ptr();
    printf("PTE |%s%s%s%s%s| %p", v, r, w, e, u, p);
  }

  PageTableEntry(const PageTableEntry&) = default;
  PageTableEntry& operator=(const PageTableEntry&) = default;
  PageTableEntry(PageTableEntry&&) = default;
  PageTableEntry& operator=(PageTableEntry&&) = default;
  ~PageTableEntry() = default;

 private:
  pte_t* ptr_;
};

}  // namespace xv6::kernel::memory::virt