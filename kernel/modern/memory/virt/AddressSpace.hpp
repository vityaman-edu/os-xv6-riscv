#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

#include <kernel/modern/memory/Address.hpp>
#include <kernel/modern/memory/Block.hpp>
#include <kernel/modern/memory/Frage.hpp>
#include <kernel/modern/memory/allocator/FrameAllocator.hpp>
#include <kernel/modern/memory/virt/PageTableEntry.hpp>
#include <kernel/modern/library/math/Number.hpp>
#include <kernel/modern/library/error/Panic.hpp>

extern "C" {
#include <kernel/legacy/defs.h>
}

namespace xv6::kernel::memory::virt {

using allocator::GlobalFrameAllocator;
using library::math::DigitsCount;

class AddressSpace {
  static constexpr std::size_t kPageTableSize = 512;
  static constexpr std::size_t kLeafLevel = 2;

 public:
  explicit AddressSpace(pagetable_t pagetable, std::size_t size)
      : pagetable_(pagetable), size_(size) {
  }

  std::optional<PageTableEntry> TranslateExisting(Virt virt) {
    return Translate(virt, false);
  }

  int DuplicateTo(AddressSpace& dst);

  void print() {
    printf("Page Table: %p\n", pagetable_);
    print(pagetable_, 0);
  }

 private:
  static void print(pagetable_t pagetable, std::size_t level);

  std::optional<PageTableEntry> Translate(Virt virt, bool alloc) {
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