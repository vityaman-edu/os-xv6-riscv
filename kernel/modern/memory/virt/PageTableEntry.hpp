#pragma once

#include "kernel/modern/memory/Address.hpp"

extern "C" {
#include <kernel/legacy/riscv.h>
#include <kernel/legacy/defs.h>
#include "kernel/legacy/types.h"
}

namespace xv6::kernel::memory::virt {

class PageTableEntry {
 public:
  explicit PageTableEntry(pte_t* const ptr) : ptr_(ptr) {
  }

  bool isValid() const {
    return ((*ptr_) & PTE_V) != 0;
  }

  bool isReadable() const {
    return ((*ptr_) & PTE_R) != 0;
  }

  bool isWrittable() const {
    return ((*ptr_) & PTE_W) != 0;
  }

  bool isExecutable() const {
    return ((*ptr_) & PTE_X) != 0;
  }

  bool isUserAccesible() const {
    return ((*ptr_) & PTE_U) != 0;
  }

  Phys physical() const {
    return Phys(PTE2PA(*ptr_));
  }

  Frame frame() const {
    return Frame(physical());
  }

  int flags() const {
    return PTE_FLAGS(*ptr_);
  }

  void print() const {
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