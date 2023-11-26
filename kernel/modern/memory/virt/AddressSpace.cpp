#include <kernel/modern/memory/virt/AddressSpace.hpp>

#define UB_ON_WRITE
#define FAULT_ON_WRITE

namespace xv6::kernel::memory::virt {

int AddressSpace::DuplicateTo(AddressSpace& dst) {
  for (UInt64 virt = 0; virt < size_; virt += Page::Size) {
    const auto maybe_pte = TranslateExisting(Virt::unsafeFrom(virt));
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
    const auto maybe_that_frame = GlobalFrameAllocator->Allocate();
    if (!maybe_that_frame.has_value()) {
      uvmunmap(dst.pagetable_, 0, virt / Page::Size, 1);
      return -1;
    }
    const auto that_frame = maybe_that_frame.value();

    memmove(
        /* dst: */ that_frame.begin().ptr(),
        /* src: */ this_frame.begin().ptr(),
        Frame::Size
    );
#else
    GlobalFrameAllocator->Reference(this_frame);
    const auto that_frame = this_frame;
#endif

    auto status = mappages(
        dst.pagetable_,
        virt,
        Frame::Size,
        that_frame.begin().toInt(),
        flags
    );

    if (status != 0) {
      GlobalFrameAllocator->Deallocate(that_frame);
      uvmunmap(dst.pagetable_, 0, virt / Frame::Size, 1);
      return -1;
    }
  }

  return 0;
}

void AddressSpace::print(pagetable_t pagetable, std::size_t level) {
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

}  // namespace xv6::kernel::memory::virt