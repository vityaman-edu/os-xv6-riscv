
extern "C" {
#include <kernel/modern/Bridge.h>
}

#include "memory/Address.hpp"
#include <kernel/modern/library/error/Panic.hpp>
#include <kernel/modern/memory/allocator/FrameAllocator.hpp>
#include <kernel/modern/memory/virt/AddressSpace.hpp>
#include <kernel/modern/memory/allocator/FrameAllocator.hpp>

using xv6::kernel::library::error::Panic;
using xv6::kernel::memory::Frame;
using xv6::kernel::memory::Phys;
using xv6::kernel::memory::Virt;
using xv6::kernel::memory::VirtChecked;
using xv6::kernel::memory::allocator::FrameAllocator;
using xv6::kernel::memory::allocator::GlobalFrameAllocator;
using xv6::kernel::memory::virt::AddressSpace;

int UserVirtMemoryCopy(
    pagetable_t src, pagetable_t dst, UInt64 size
) {
  auto source = AddressSpace(src, size);
  auto destination = AddressSpace(dst, size);
  return source.CopyTo(destination);
}

void UserVirtMemoryDump(pagetable_t pagetable, UInt64 size) {
  AddressSpace(pagetable, size).print();
}

int UVMCOWFixPageFault(
    pagetable_t pagetable, UInt64 size, UInt64 virt
) {
  auto maybe_valid_virt = VirtChecked(virt);
  if (!maybe_valid_virt.has_value()) {
    return -5;
  }
  auto valid_virt = maybe_valid_virt.value();

  auto space = AddressSpace(pagetable, size);
  if (auto pte = space.TranslateExisting(valid_virt)) {
#ifdef COW_DEBUG
    printf("[debug] Page fault on %p backed by\n", virt);
    pte->print();
    printf("\n");
    space.print();
#endif


    if (!pte->isValid()) {
      return -2;
    }

    if (pte->isWrittable()) {
      return -3;
    }

    if (pte->isExecutable()) {
      return -6;
    }

    auto count = GlobalFrameAllocator->ReferencesCount(pte->frame());
    if (count == 0) {
      Panic("Frame in a valid PTE must be reserved");
    }

    if (1 < count) {
      if (auto that_frame = GlobalFrameAllocator->Allocate()) {
        auto this_frame = pte->frame();
        memmove(
            /* dst: */ that_frame->begin().ptr(),
            /* src: */ this_frame.begin().ptr(),
            Frame::Size
        );
        pte->setFrame(*that_frame);
        GlobalFrameAllocator->Deallocate(this_frame);
      } else {
        return -4;
      }
    }

    pte->setWrittable(true);

#ifdef COW_DEBUG
    printf("[debug] Page fault fix on %p backed by\n", virt);
    pte->print();
    printf("\n");
    space.print();
#endif

    return 0;
  }

  return -1;
}

void GlobalFrameAllocatorInit() {
  GlobalFrameAllocator
      = (FrameAllocator*)bd_malloc(sizeof(FrameAllocator));
  new (GlobalFrameAllocator) FrameAllocator();
}

void* GlobalFrameAllocatorAllocate() {
  if (const auto frame = GlobalFrameAllocator->Allocate()) {
    return frame->begin().ptr();
  }
  return nullptr;
}

void GlobalFrameAllocatorDeallocate(void* phys) {
  const auto frame = Frame(Phys::unsafeFrom((std::uint64_t)phys));
  GlobalFrameAllocator->Deallocate(frame);
}
