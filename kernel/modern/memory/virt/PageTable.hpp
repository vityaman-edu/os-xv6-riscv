#pragma once

// #define UB_ON_WRITE

#include <cstddef>
#include <optional>

#include <kernel/modern/memory/address/Address.hpp>
#include <kernel/modern/memory/virt/PageTableEntry.hpp>

extern "C" {
#include <kernel/defs.h>
}

namespace xv6::kernel::memory::virt {

using address::Virt;

constexpr std::size_t kPageSize = 4096;

class PageTable {
 public:
  explicit PageTable(pagetable_t pagetable, std::size_t size)
      : pagetable_(pagetable), size_(size) {
  }

  auto TranslateExisting(address::Virt virt)
      -> std::optional<PTERef> {
    return Translate(virt, false);
  }

  auto CopyTo(PageTable& dst) -> int {
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
      const auto flags = pte.Flags();

#ifndef UB_ON_WRITE
      auto* const frame_ptr = kalloc();
      if (frame_ptr == nullptr) {
        uvmunmap(dst.pagetable_, 0, virt / kPageSize, 1);
        return -1;
      }
      memmove(frame_ptr, phys.AsPtr(), kPageSize);
#else
      // auto* const frame_ptr = phys.AsPtr();
#endif

      if (mappages(
              dst.pagetable_,
              virt,
              kPageSize,
              (UInt64)frame_ptr,
              flags
          )
          != 0) {
        kfree(frame_ptr);
        uvmunmap(dst.pagetable_, 0, virt / kPageSize, 1);
        return -1;
      }
    }

    return 0;
  }

 private:
  auto Translate(address::Virt virt, bool alloc)
      -> std::optional<PTERef> {
    pte_t* const ptr = translate(
        pagetable_, virt.ToUInt64(), static_cast<int>(alloc)
    );
    if (ptr == nullptr) {
      return std::nullopt;
    }
    return PTERef(ptr);
  }

  pagetable_t pagetable_;
  std::size_t size_;
};

}  // namespace xv6::kernel::memory::virt