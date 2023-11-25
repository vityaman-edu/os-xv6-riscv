#pragma once

// #define UB_ON_WRITE
// #define FAULT_ON_WRITE

#include <algorithm>
#include <cstddef>
#include <optional>

#include <kernel/modern/memory/address/Address.hpp>
#include <kernel/modern/memory/virt/PageTableEntry.hpp>
#include <kernel/modern/library/math/Number.hpp>

extern "C" {
#include <kernel/defs.h>
}

namespace xv6::kernel::memory::virt {

using address::Virt;
using library::math::DigitsCount;

class AddressSpace {
  static constexpr std::size_t kPageTableSize = 512;
  static constexpr std::size_t kPageSize = 4096;
  static constexpr std::size_t kLeafLevel = 3;

 public:
  explicit AddressSpace(pagetable_t pagetable, std::size_t size)
      : pagetable_(pagetable), size_(size) {
  }

  auto TranslateExisting(address::Virt virt)
      -> std::optional<PageTableEntry> {
    return Translate(virt, false);
  }

  auto CopyTo(AddressSpace& dst) -> int {
    for (UInt64 virt = 0; virt < size_; virt += kPageSize) {
      const auto maybe_pte = TranslateExisting(Virt(virt));
      if (!maybe_pte.has_value()) {
        panic("uvmcopy: pte should exist");
      }

      const auto pte = maybe_pte.value();
      if (!pte.IsValid()) {
        panic("uvmcopy: page not present");
      }

      const auto phys = pte.Physical();

#ifndef FAULT_ON_WRITE
      const auto flags = pte.Flags();
#else
      const auto flags = pte.Flags() & ~PTE_W;
#endif


#ifndef UB_ON_WRITE
      auto* const frame_ptr = kalloc();
      if (frame_ptr == nullptr) {
        uvmunmap(dst.pagetable_, 0, virt / kPageSize, 1);
        return -1;
      }
      memmove(frame_ptr, phys.AsPtr(), kPageSize);
#else
      auto* const frame_ptr = phys.AsPtr();
#endif

      auto status = mappages(
          dst.pagetable_, virt, kPageSize, (UInt64)frame_ptr, flags
      );
      if (status != 0) {
        kfree(frame_ptr);
        uvmunmap(dst.pagetable_, 0, virt / kPageSize, 1);
        return -1;
      }
    }

    return 0;
  }

  auto Print() -> void {
    printf("Page Table: %p\n", pagetable_);
    Print(pagetable_, 0);
  }

 private:
  static auto Print(pagetable_t pagetable, std::size_t level)
      -> void {
    if (level == kLeafLevel + 1) {
      return;
    }
    for (std::size_t i = 0; i < kPageTableSize; ++i) {
      const auto pte = PageTableEntry(&pagetable[i]);
      if (pte.IsValid()) {
        for (std::size_t i = 0; i < level; ++i) {
          printf(".. ");
        }

        const auto* space = "";
        switch (DigitsCount(i)) {
          case 2: {
            space = "0";
          } break;
          case 1: {
            space = "00";
          } break;
          default: {
          } break;
        }
        printf("%s%d: ", space, i);
        pte.Print();
        printf("\n");

        Print((pagetable_t)pte.Physical().ToUInt64(), level + 1);
      }
    }
  }

  auto Translate(address::Virt virt, bool alloc)
      -> std::optional<PageTableEntry> {
    pte_t* const ptr = translate(
        pagetable_, virt.ToUInt64(), static_cast<int>(alloc)
    );
    if (ptr == nullptr) {
      return std::nullopt;
    }
    return PageTableEntry(ptr);
  }

  pagetable_t pagetable_;
  std::size_t size_;
};

}  // namespace xv6::kernel::memory::virt