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

  /// @return true only if present
  [[nodiscard]] bool isValid() const {
    return ((*ptr_) & PTE_V) != 0;
  }

  [[nodiscard]] bool isReadable() const {
    return ((*ptr_) & PTE_R) != 0;
  }

  [[nodiscard]] bool isWrittable() const {
    return ((*ptr_) & PTE_W) != 0;
  }

  void setWrittable(bool status) {
    if (status) {
      (*ptr_) = (*ptr_) | PTE_W;
    } else {
      (*ptr_) = (*ptr_) & ~PTE_W;
    }
  }

  [[nodiscard]] bool isExecutable() const {
    return ((*ptr_) & PTE_X) != 0;
  }

  [[nodiscard]] bool isUserAccesible() const {
    return ((*ptr_) & PTE_U) != 0;
  }

  [[nodiscard]] bool isCopiedOnWrite() const {
    return ((*ptr_) & PTE_COW) != 0;
  }

  void setCopiedOnWrite(bool status) const {
    if (status) {
      (*ptr_) |= PTE_COW;
    } else {
      (*ptr_) &= ~PTE_COW;
    }
  }

  [[nodiscard]] Phys physical() const {
    return Phys::unsafeFrom(PTE2PA(*ptr_));
  }

  [[nodiscard]] Frame frame() const {
    return Frame(physical());
  }

  void setFrame(Frame frame) {
    auto phys = frame.begin().toInt();
    (*ptr_) = 0x0 | PA2PTE(phys) | flags();
  }

  [[nodiscard]] int flags() const {
    return PTE_FLAGS(*ptr_);
  }

  void print() const {
    const auto* v = isValid() ? "V" : " ";
    const auto* r = isReadable() ? "R" : " ";
    const auto* w = isWrittable() ? "W" : " ";
    const auto* e = isExecutable() ? "X" : " ";
    const auto* u = isUserAccesible() ? "U" : " ";
    const auto* c = isCopiedOnWrite() ? "C" : " ";
    const auto p = physical().ptr();
    printf("PTE |%s%s%s%s%s%s| %p", v, r, w, e, u, c, p);
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