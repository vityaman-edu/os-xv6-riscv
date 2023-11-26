#pragma once

// #define UB_ON_WRITE
// #define FAULT_ON_WRITE

#include <algorithm>
#include <cstddef>
#include <optional>

#include <kernel/modern/memory/Address.hpp>
#include <kernel/modern/memory/allocator/FrameAllocator.hpp>
#include <kernel/modern/memory/virt/PageTableEntry.hpp>
#include <kernel/modern/library/math/Number.hpp>
#include <kernel/modern/library/error/Panic.hpp>

extern "C" {
#include <kernel/defs.h>
}

namespace xv6::kernel::memory::virt {

using allocator::FrameAllocator;
using library::math::DigitsCount;

class AddressSpace {
  static constexpr std::size_t kPageTableSize = 512;
  static constexpr std::size_t kLeafLevel = 3;

 public:
  explicit AddressSpace(pagetable_t pagetable, std::size_t size)
      : pagetable_(pagetable), size_(size) {
  }

  auto TranslateExisting(Virt virt) -> std::optional<PageTableEntry> {
    return Translate(virt, false);
  }

  auto CopyTo(AddressSpace& dst) -> int {
    for (UInt64 virt = 0; virt < size_; virt += Page::kSize) {
      const auto maybe_pte = TranslateExisting(Virt(virt));
      if (!maybe_pte.has_value()) {
        Panic("uvmcopy: pte should exist");
      }

      const auto pte = maybe_pte.value();
      if (!pte.isValid()) {
        Panic("uvmcopy: page not present");
      }

      const auto this_frame = pte.frame();

#ifndef FAULT_ON_WRITE
      const auto flags = pte.flags();
#else
      const auto flags = pte.flags() & ~PTE_W;
#endif

#ifndef UB_ON_WRITE
      const auto maybe_that_frame = FrameAllocator::Allocate();
      if (!maybe_that_frame.has_value()) {
        uvmunmap(dst.pagetable_, 0, virt / Page::kSize, 1);
        return -1;
      }
      const auto that_frame = maybe_that_frame.value();

      memmove(
          /* dst: */ that_frame.begin().ptr(),
          /* src: */ this_frame.begin().ptr(),
          Frame::kSize
      );
#else
      const auto that_frame = this_frame;
#endif

      auto status = mappages(
          dst.pagetable_,
          virt,
          Frame::kSize,
          that_frame.begin().toInt(),
          flags
      );
      if (status != 0) {
        FrameAllocator::Deallocate(that_frame);
        uvmunmap(dst.pagetable_, 0, virt / Frame::kSize, 1);
        return -1;
      }
    }

    return 0;
  }

  auto print() -> void {
    printf("Page Table: %p\n", pagetable_);
    print(pagetable_, 0);
  }

 private:
  static auto print(pagetable_t pagetable, std::size_t level)
      -> void {
    if (level == kLeafLevel + 1) {
      return;
    }

    for (std::size_t i = 0; i < kPageTableSize; ++i) {
      const auto pte = PageTableEntry(&pagetable[i]);
      if (pte.isValid()) {
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
        pte.print();
        printf("\n");

        print((pagetable_t)pte.physical().toInt(), level + 1);
      }
    }
  }

  auto Translate(Virt virt, bool alloc)
      -> std::optional<PageTableEntry> {
    auto* const pte = translate(
        pagetable_, virt.toInt(), static_cast<int>(alloc)
    );
    if (pte == nullptr) {
      return std::nullopt;
    }
    return PageTableEntry(pte);
  }

  pagetable_t pagetable_;
  std::size_t size_;
};

}  // namespace xv6::kernel::memory::virt