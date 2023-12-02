#include <kernel/alloc/frame_allocator.h>
#include <kernel/core/flag.h>
#include <kernel/core/param.h>
#include <kernel/core/result.h>
#include <kernel/core/type.h>
#include <kernel/defs.h>
#include <kernel/file/fs.h>
#include <kernel/hw/arch/riscv/register.h>
#include <kernel/hw/memlayout.h>
#include <kernel/memory/vm.h>
#include <kernel/process/elf.h>

/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;

extern char etext[]; // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

// Make a direct-map page table for the kernel.
pagetable_t kvmmake(void) {
  pagetable_t kpgtbl = (pagetable_t)frame_allocate().ptr;

  memset(kpgtbl, 0, PGSIZE);

  // uart registers
  kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  kvmmap(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // PLIC
  kvmmap(kpgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)etext - KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  kvmmap(
      kpgtbl,
      (uint64)etext,
      (uint64)etext,
      PHYSTOP - (uint64)etext,
      PTE_R | PTE_W
  );

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

  // allocate and map a kernel stack for each process.
  proc_mapstacks(kpgtbl);

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

bool virt_is_valid(virt virt) {
  return virt < MAXVA;
}

frame pte_frame(pte_t pte) {
  return frame_parse((void*)PTE2PA(pte));
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
pte_t* vmwalk(pagetable_t pagetable, uint64 va, int alloc) {
  if (!virt_is_valid(va)) {
    panic("walk");
  }

  for (int level = 2; level > 0; level--) {
    pte_t* pte = &pagetable[PX(level, va)];
    if (*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if (!alloc || (pagetable = (pde_t*)frame_allocate().ptr) == 0) {
        return 0;
      }
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64 vmwalkaddr(pagetable_t pagetable, uint64 va) {
  pte_t* pte;
  uint64 pa;

  if (va >= MAXVA) {
    return 0;
  }

  pte = vmwalk(pagetable, va, 0);
  if (pte == 0) {
    return 0;
  }
  if ((*pte & PTE_V) == 0) {
    return 0;
  }
  if ((*pte & PTE_U) == 0) {
    return 0;
  }
  pa = PTE2PA(*pte);
  return pa;
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
  if (vmmappages(kpgtbl, va, sz, pa, perm) != 0) {
    panic("kvmmap");
  }
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int vmmappages(
    pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm
) {
  uint64 a, last;
  pte_t* pte;

  if (size == 0) {
    panic("mappages: size");
  }

  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for (;;) {
    if ((pte = vmwalk(pagetable, a, 1)) == 0) {
      return -1;
    }
    if (*pte & PTE_V) {
      panic("mappages: remap");
    }
    *pte = PA2PTE(pa) | perm | PTE_V;
    if (a == last) {
      break;
    }
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free) {
  uint64 a;
  pte_t* pte;

  if ((va % PGSIZE) != 0) {
    panic("uvmunmap: not aligned");
  }

  for (a = va; a < va + npages * PGSIZE; a += PGSIZE) {
    if ((pte = vmwalk(pagetable, a, 0)) == 0) {
      panic("uvmunmap: walk");
    }
    if ((*pte & PTE_V) == 0) {
      panic("uvmunmap: not mapped");
    }
    if (PTE_FLAGS(*pte) == PTE_V) {
      panic("uvmunmap: not a leaf");
    }
    if (do_free) {
      uint64 pa = PTE2PA(*pte);
      frame_free(frame_parse((void*)pa));
    }
    *pte = 0;
  }
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t uvmcreate() {
  pagetable_t pagetable = (pagetable_t)frame_allocate().ptr;
  if (pagetable == 0) {
    return 0;
  }
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// Load the user initcode into address 0 of pagetable,
// for the very first process.
// sz must be less than a page.
void uvmfirst(pagetable_t pagetable, uchar* src, uint sz) {
  char* mem;

  if (sz >= PGSIZE) {
    panic("uvmfirst: more than a page");
  }
  mem = frame_allocate().ptr;
  memset(mem, 0, PGSIZE);
  vmmappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W | PTE_R | PTE_X | PTE_U);
  memmove(mem, src, sz);
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int xperm) {
  char* mem;
  uint64 a;

  if (newsz < oldsz) {
    return oldsz;
  }

  oldsz = PGROUNDUP(oldsz);
  for (a = oldsz; a < newsz; a += PGSIZE) {
    mem = frame_allocate().ptr;
    if (mem == 0) {
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if (vmmappages(pagetable, a, PGSIZE, (uint64)mem, PTE_R | PTE_U | xperm)
        != 0) {
      frame_free(frame_parse(mem));
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz) {
  if (newsz >= oldsz) {
    return oldsz;
  }

  if (PGROUNDUP(newsz) < PGROUNDUP(oldsz)) {
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
  }

  return newsz;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void freewalk(pagetable_t pagetable) {
  // there are 2^9 = 512 PTEs in a page table.
  for (int i = 0; i < 512; i++) {
    pte_t pte = pagetable[i];
    if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0) {
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if (pte & PTE_V) {
      panic("freewalk: leaf");
    }
  }
  frame_free(frame_parse((void*)pagetable));
}

// Free user memory pages,
// then free page-table pages.
void uvmfree(pagetable_t pagetable, uint64 sz) {
  if (sz > 0) {
    uvmunmap(pagetable, 0, PGROUNDUP(sz) / PGSIZE, 1);
  }
  freewalk(pagetable);
}

/// Given a parent process's page table, copy its memory into a
/// child's page table. Copies both the page table and the
/// physical memory. returns 0 on success, -1 on failure.
/// frees any allocated pages on failure.
rstatus_t uvmcopy(pagetable_t oldpt, pagetable_t newpt, uint64 size) {
  for (virt virt = 0; virt < size; virt += PGSIZE) {
    pte_t* pte = vmwalk(oldpt, virt, 0);
    if (pte == nullptr) {
      panic("uvmcopy: pte should exist");
    }

    if (FLAG_DISABLED(*pte, PTE_V)) {
      panic("uvmcopy: page not present");
    }

    if (FLAG_ENABLED(*pte, PTE_W)) {
      *pte &= ~PTE_W;
      *pte |= PTE_COW;
    }

    const frame old_frame = pte_frame(*pte);
    frame_reference(old_frame);

    if (vmmappages(newpt, virt, PGSIZE, old_frame.addr, PTE_FLAGS(*pte)) != 0) {
      const uint64 start = 0;
      const uint64 npages = (virt / PGSIZE);
      const bool do_free = true;
      uvmunmap(newpt, start, npages, do_free); // Rollback
      return BAD_ALLOC;
    }
  }

  return OK;
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void uvmclear(pagetable_t pagetable, uint64 va) {
  pte_t* pte;

  pte = vmwalk(pagetable, va, 0);
  if (pte == 0) {
    panic("uvmclear");
  }
  *pte &= ~PTE_U;
}

rstatus_t uvm_handle_page_fault(pagetable_t pagetable, virt virt) {
  if (!virt_is_valid(virt)) {
    return NOT_FOUND;
  }

  pte_t* pte = vmwalk(pagetable, virt, /*alloc*/ false);
  if (pte == nullptr) {
    return NOT_FOUND;
  }

  if (FLAG_ENABLED(*pte, PTE_COW)) {
    return uvm_copy_on_write(pte);
  }

  return UNKNOWN;
}

rstatus_t uvm_copy_on_write(pte_t* pte) {
  if (FLAG_DISABLED(*pte, PTE_COW)) {
    panic("uvm_cow: pte must be COW");
  }
  if (pte == nullptr) {
    panic("uvm_cow: pte is null");
  }

  const frame old_frame = pte_frame(*pte);

  const ref_count_t ref_count = frame_ref_count(old_frame);
  if (ref_count == 0) {
    panic("uvm_cow: ref_count = 0");
  } else if (ref_count == 1) {
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
  } else {
    uint new_flags = PTE_FLAGS(*pte);
    new_flags |= PTE_W;
    new_flags &= ~PTE_COW;

    const frame new_frame = frame_allocate();
    if (!new_frame.is_valid) {
      return BAD_ALLOC;
    }

    memmove(/*dst*/ new_frame.ptr, /*src*/ old_frame.ptr, PGSIZE);

    *pte = PA2PTE(new_frame.addr) | new_flags;

    frame_free(old_frame);
  }

  return OK;
}

/// Copy from kernel to user.
/// Copy len bytes from src to virtual address dstva in a given page table.
rstatus_t
vmcopyout(pagetable_t pagetable, uint64 dstva, char* src, uint64 len) {
  while (len > 0) {
    const virt va0 = PGROUNDDOWN(dstva);
    if (!virt_is_valid(va0)) {
      return NOT_FOUND;
    }

    pte_t* pte = vmwalk(pagetable, va0, /*alloc:*/ false);
    if (pte == nullptr) {
      return NOT_FOUND;
    }
    if (FLAG_DISABLED(*pte, PTE_V)) {
      return NOT_FOUND;
    }
    if (FLAG_DISABLED(*pte, PTE_U)) {
      return PERMISSION_DENIED;
    }

    if (FLAG_ENABLED(*pte, PTE_COW)) {
      rstatus_t status = uvm_copy_on_write(pte);
      if (status != OK) {
        return status;
      }
    }

    const phys pa0 = vmwalkaddr(pagetable, va0);
    if (pa0 == 0) {
      return NOT_FOUND;
    }

    uint64 rem = PGSIZE - (dstva - va0);
    if (rem > len) {
      rem = len;
    }

    memmove((void*)(pa0 + (dstva - va0)), src, rem);

    len -= rem;
    src += rem;

    dstva = va0 + PGSIZE;
  }

  return OK;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int vmcopyin(pagetable_t pagetable, char* dst, uint64 srcva, uint64 len) {
  uint64 n, va0, pa0;

  while (len > 0) {
    va0 = PGROUNDDOWN(srcva);
    pa0 = vmwalkaddr(pagetable, va0);
    if (pa0 == 0) {
      return -1;
    }
    n = PGSIZE - (srcva - va0);
    if (n > len) {
      n = len;
    }
    memmove(dst, (void*)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int vmcopyinstr(pagetable_t pagetable, char* dst, uint64 srcva, uint64 max) {
  uint64 n, va0, pa0;
  int got_null = 0;

  while (got_null == 0 && max > 0) {
    va0 = PGROUNDDOWN(srcva);
    pa0 = vmwalkaddr(pagetable, va0);
    if (pa0 == 0) {
      return -1;
    }
    n = PGSIZE - (srcva - va0);
    if (n > max) {
      n = max;
    }

    char* p = (char*)(pa0 + (srcva - va0));
    while (n > 0) {
      if (*p == '\0') {
        *dst = '\0';
        got_null = 1;
        break;
      }

      *dst = *p;

      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;
  }
  if (got_null) {
    return 0;
  }
  return -1;
}

void pte_print(pte_t pte) {
  const char* v = FLAG_ENABLED(pte, PTE_V) /*  */ ? "V" : " ";
  const char* r = FLAG_ENABLED(pte, PTE_R) /*  */ ? "R" : " ";
  const char* w = FLAG_ENABLED(pte, PTE_W) /*  */ ? "W" : " ";
  const char* e = FLAG_ENABLED(pte, PTE_X) /*  */ ? "X" : " ";
  const char* u = FLAG_ENABLED(pte, PTE_U) /*  */ ? "U" : " ";
  const char* c = FLAG_ENABLED(pte, PTE_COW) /**/ ? "C" : " ";
  const void* p = pte_frame(pte).ptr;
  printf("PTE |%s%s%s%s%s%s| %p", v, r, w, e, u, c, p);
}

void vm_print_level(pagetable_t pagetable, size_t level) {
  if (level == PAGETABLE_DEPTH) {
    return;
  }

  for (index_t i = 0; i < PAGETABLE_SIZE; ++i) {
    const pte_t* pte = &pagetable[i];
    if (FLAG_DISABLED(*pte, PTE_V)) {
      continue;
    }

    for (index_t i = 0; i < level; ++i) {
      printf(".. ");
    }
    const char* space = (100 <= i) ? "" : ((10 <= i) ? "0" : "00");
    printf("%s%d: ", space, i);
    pte_print(*pte);
    printf("\n");

    vm_print_level((pagetable_t)PTE2PA(*pte), level + 1);
  }
}

void vm_print(pagetable_t pagetable) {
  printf("Page Table: %p\n", pagetable);
  vm_print_level(pagetable, 0);
}