#pragma once

#include "kernel/modern/memory/address/Address.hpp"

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

  auto IsValid() const -> bool {
    return ((*ptr_) & PTE_V) != 0;
  }

  auto IsReadable() const -> bool {
    return ((*ptr_) & PTE_R) != 0;
  }

  auto IsWrittable() const -> bool {
    return ((*ptr_) & PTE_W) != 0;
  }

  auto IsExecutable() const -> bool {
    return ((*ptr_) & PTE_X) != 0;
  }

  auto IsUserAccesible() const -> bool {
    return ((*ptr_) & PTE_U) != 0;
  }

  auto Physical() const -> address::Phys {
    return address::Phys(PTE2PA(*ptr_));
  }

  auto Flags() const -> int {
    return PTE_FLAGS(*ptr_);
  }

  auto Print() const -> void {
    const auto* v = !IsValid() ? "INVALID " : "";
    const auto* r = IsReadable() ? "R" : " ";
    const auto* w = IsWrittable() ? "W" : " ";
    const auto* e = IsExecutable() ? "E" : "D";
    const auto* u = IsUserAccesible() ? "U" : "K";
    const auto p = Physical().AsPtr();
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