#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"

#include <kernel/modern/Bridge.h>

/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;

extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[];  // trampoline.S

// Make a direct-map page table for the kernel.
pagetable_t kvmmake(void) {
  pagetable_t kpgtbl = 0;

  kpgtbl = (pagetable_t)kalloc();
  memset(kpgtbl, 0, PGSIZE);

  // uart registers
  kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  kvmmap(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // PLIC
  kvmmap(kpgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  kvmmap(
      kpgtbl,
      KERNBASE,
      KERNBASE,
      (UInt64)etext - KERNBASE,
      PTE_R | PTE_X
  );

  // map kernel data and the physical RAM we'll make use of.
  kvmmap(
      kpgtbl,
      (UInt64)etext,
      (UInt64)etext,
      PHYSTOP - (UInt64)etext,
      PTE_R | PTE_W
  );

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(
      kpgtbl, TRAMPOLINE, (UInt64)trampoline, PGSIZE, PTE_R | PTE_X
  );

  // allocate and map a kernel stack for each process.
  // proc_mapstacks(kpgtbl);

  return kpgtbl;
}

// Initialize the one kernel_pagetable
void kvminit(void) {
  kernel_pagetable = kvmmake();
}

// Switch h/w page table register to the kernel's page table,
// and enable paging.
void kvminithart() {
  // wait for any previous writes to the page table memory to finish.
  sfence_vma();

  w_satp(MAKE_SATP(kernel_pagetable));

  // flush stale entries from the TLB.
  sfence_vma();
}

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t* translate(pagetable_t pagetable, UInt64 virt, int alloc) {
  if (virt >= MAXVA) {
    panic("walk");
  }

  for (int level = 2; level > 0; level--) {
    pte_t* pte = &pagetable[PX(level, virt)];
    if (*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if (!alloc || (pagetable = (pde_t*)kalloc()) == 0) {
        return 0;
      }
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }

  return &pagetable[PX(0, virt)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
UInt64 walkaddr(pagetable_t pagetable, UInt64 virt) {
  if (virt >= MAXVA) {
    return 0;
  }

  const pte_t* pte = translate(pagetable, virt, 0);
  if (pte == 0) {
    return 0;
  }
  if ((*pte & PTE_V) == 0) {
    return 0;
  }
  if ((*pte & PTE_U) == 0) {
    return 0;
  }

  UInt64 phys = PTE2PA(*pte);
  return phys;
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void kvmmap(
    pagetable_t kpgtbl,
    UInt64 virt,
    UInt64 phys,
    UInt64 size,
    int perm
) {
  if (mappages(kpgtbl, virt, size, phys, perm) != 0) {
    panic("kvmmap");
  }
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int mappages(
    pagetable_t pagetable,
    UInt64 virt,
    UInt64 size,
    UInt64 phys,
    int perm
) {
  UInt64 addr = 0;
  UInt64 last = 0;
  pte_t* pte = 0;

  if (size == 0) {
    panic("mappages: size");
  }

  addr = PGROUNDDOWN(virt);
  last = PGROUNDDOWN(virt + size - 1);
  for (;;) {
    if ((pte = translate(pagetable, addr, 1)) == 0) {
      return -1;
    }

    if (*pte & PTE_V) {
      panic("mappages: remap");
    }

    *pte = PA2PTE(phys) | perm | PTE_V;

    if (addr == last) {
      break;
    }

    addr += PGSIZE;
    phys += PGSIZE;
  }

  return 0;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void uvmunmap(
    pagetable_t pagetable, UInt64 virt, UInt64 npages, int do_free
) {
  UInt64 addr = 0;
  pte_t* pte = 0;

  if ((virt % PGSIZE) != 0) {
    panic("uvmunmap: not aligned");
  }

  for (addr = virt; addr < virt + npages * PGSIZE; addr += PGSIZE) {
    if ((pte = translate(pagetable, addr, 0)) == 0) {
      panic("uvmunmap: walk");
    }
    if ((*pte & PTE_V) == 0) {
      panic("uvmunmap: not mapped");
    }
    if (PTE_FLAGS(*pte) == PTE_V) {
      panic("uvmunmap: not a leaf");
    }
    if (do_free) {
      UInt64 phys = PTE2PA(*pte);
      kfree((void*)phys);
    }
    *pte = 0;
  }
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t uvmcreate() {
  pagetable_t pagetable = 0;
  pagetable = (pagetable_t)kalloc();
  if (pagetable == 0) {
    return 0;
  }
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// Load the user initcode into address 0 of pagetable,
// for the very first process.
// sz must be less than a page.
void uvmfirst(pagetable_t pagetable, UChar* src, UInt32 size) {
  char* mem = 0;

  if (size >= PGSIZE) {
    panic("uvmfirst: more than a page");
  }
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(
      pagetable, 0, PGSIZE, (UInt64)mem, PTE_W | PTE_R | PTE_X | PTE_U
  );
  memmove(mem, src, size);
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on
// error.
UInt64 uvmalloc(
    pagetable_t pagetable, UInt64 old_size, UInt64 new_size, int xperm
) {
  char* mem = 0;
  UInt64 addr = 0;

  if (new_size < old_size) {
    return old_size;
  }

  old_size = PGROUNDUP(old_size);
  for (addr = old_size; addr < new_size; addr += PGSIZE) {
    mem = kalloc();
    if (mem == 0) {
      uvmdealloc(pagetable, addr, old_size);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if (mappages(
            pagetable,
            addr,
            PGSIZE,
            (UInt64)mem,
            PTE_R | PTE_U | xperm
        )
        != 0) {
      kfree(mem);
      uvmdealloc(pagetable, addr, old_size);
      return 0;
    }
  }
  return new_size;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
UInt64 uvmdealloc(pagetable_t pagetable, UInt64 oldsz, UInt64 newsz) {
  if (newsz >= oldsz) {
    return oldsz;
  }

  if (PGROUNDUP(newsz) < PGROUNDUP(oldsz)) {
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
  }

  return newsz;
}

// there are 2^9 = 512 PTEs in a page table.
#define PAGETABLE_SIZE 512

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void freewalk(pagetable_t pagetable) {
  for (int i = 0; i < PAGETABLE_SIZE; i++) {
    pte_t pte = pagetable[i];
    if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0) {
      // this PTE points to a lower-level page table.
      UInt64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if (pte & PTE_V) {
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}

// Free user memory pages,
// then free page-table pages.
void uvmfree(pagetable_t pagetable, UInt64 size) {
  if (size > 0) {
    uvmunmap(pagetable, 0, PGROUNDUP(size) / PGSIZE, 1);
  }
  freewalk(pagetable);
}

// Given a parent process's page table, copy
// its memory into a child's page table.
// Copies both the page table and the
// physical memory.
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int uvmcopy(pagetable_t src, pagetable_t dst, UInt64 size) {
  return UserVirtMemoryCopy(src, dst, size);

  // for (UInt64 virt = 0; virt < size; virt += PGSIZE) {
  //   pte_t const* const pte_ptr = translate(src, virt, 0);
  //   if (pte_ptr == 0) {
  //     panic("uvmcopy: pte should exist");
  //   }

  //   pte_t const pte = *pte_ptr;
  //   if ((pte & PTE_V) == 0) {
  //     panic("uvmcopy: page not present");
  //   }

  //   UInt64 const phys = PTE2PA(pte);
  //   UInt32 const flags = PTE_FLAGS(pte);

  //   Byte* const new_mem = kalloc();
  //   if (new_mem == 0) {
  //     uvmunmap(dst, 0, virt / PGSIZE, 1);
  //     return -1;
  //   }

  //   memmove(new_mem, (Byte*)phys, PGSIZE);
  //   if (mappages(dst, virt, PGSIZE, (UInt64)new_mem, flags) != 0) {
  //     kfree(new_mem);

  //     uvmunmap(dst, 0, virt / PGSIZE, 1);
  //     return -1;
  //   }
  // }

  // return 0;
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void uvmclear(pagetable_t pagetable, UInt64 virt) {
  pte_t* pte = translate(pagetable, virt, 0);
  if (pte == 0) {
    panic("uvmclear");
  }
  *pte &= ~PTE_U;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page
// table. Return 0 on success, -1 on error.
int copyout(
    pagetable_t pagetable, UInt64 dstva, char* src, UInt64 len
) {
  //   |     i     |   i + 1   |   i + 2   | ... |   i + k   |
  // . | . . x x x | x x x x x | x x x x x | ... | x x . . . | . . .

  while (len > 0) {
    const UInt64 virt0 = PGROUNDDOWN(dstva);

    {
      if (virt0 >= MAXVA) {
        return -1;
      }

      pte_t* pte = translate(pagetable, virt0, 0);
      if (pte == 0 || (*pte & PTE_U) == 0 || (*pte & PTE_V) == 0) {
        return -1;
      }

      if (((*pte) & PTE_COW) != 0) {
        UInt32 flags = PTE_FLAGS(*pte);
        flags |= PTE_W;
        flags &= ~PTE_COW;

        UInt64 phys = PTE2PA(*pte);
        char* mem = (char*)kalloc();
        if (mem == 0) {
          return -2;
        }

        memmove(mem, (char*)phys, PGSIZE);

        *pte = PA2PTE(mem) | flags;

        kfree((char*)phys);
      }
    }

    const UInt64 phys0 = walkaddr(pagetable, virt0);
    if (phys0 == 0) {
      return -1;
    }

    const UInt64 shift = dstva - virt0;

    UInt64 nbytes = PGSIZE - shift;
    if (nbytes > len) {
      nbytes = len;
    }

    memmove((void*)(phys0 + shift), src, nbytes);

    len -= nbytes;
    src += nbytes;

    dstva = virt0 + PGSIZE;
  }

  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page
// table. Return 0 on success, -1 on error.
int copyin(
    pagetable_t pagetable, char* dst, UInt64 srcva, UInt64 len
) {
  UInt64 n = 0;  // NOLINT
  UInt64 virt0 = 0;
  UInt64 phys0 = 0;

  while (len > 0) {
    virt0 = PGROUNDDOWN(srcva);

    phys0 = walkaddr(pagetable, virt0);
    if (phys0 == 0) {
      return -1;
    }

    n = PGSIZE - (srcva - virt0);
    if (n > len) {
      n = len;
    }

    memmove(dst, (void*)(phys0 + (srcva - virt0)), n);

    len -= n;
    dst += n;
    srcva = virt0 + PGSIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int copyinstr(
    pagetable_t pagetable, char* dst, UInt64 srcva, UInt64 max
) {
  UInt64 n = 0;  // NOLINT
  UInt64 virt0 = 0;
  UInt64 phys0 = 0;
  int got_null = 0;

  while (got_null == 0 && max > 0) {
    virt0 = PGROUNDDOWN(srcva);
    phys0 = walkaddr(pagetable, virt0);
    if (phys0 == 0) {
      return -1;
    }

    n = PGSIZE - (srcva - virt0);
    if (n > max) {
      n = max;
    }

    char* phys = (char*)(phys0 + (srcva - virt0));
    while (n > 0) {
      if (*phys == '\0') {
        *dst = '\0';
        got_null = 1;
        break;
      }

      *dst = *phys;

      --n;
      --max;
      phys++;
      dst++;
    }

    srcva = virt0 + PGSIZE;
  }
  if (got_null) {
    return 0;
  }
  return -1;
}
