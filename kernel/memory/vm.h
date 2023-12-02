#pragma once

#define PAGETABLE_SIZE 512
#define PAGETABLE_DEPTH 3
#define PAGETABLE_LEAF_LEVEL ((PAGETABLE_DEPTH)-1)

#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))

#define PTE_V (1L << 0)   // is valid
#define PTE_R (1L << 1)   // is readable
#define PTE_W (1L << 2)   // is writtable
#define PTE_X (1L << 3)   // is executable
#define PTE_U (1L << 4)   // is user accessible
#define PTE_COW (1L << 8) // is writtable, but can be copied on write

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte) >> 10) << 12)

#define PTE_FLAGS(pte) ((pte)&0x3FF)

// extract the three 9-bit page table indices from a virtual address.
#define PXMASK 0x1FF // 9 bits
#define PXSHIFT(level) (PGSHIFT + (9 * (level)))
#define PX(level, va) ((((uint64)(va)) >> PXSHIFT(level)) & PXMASK)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))

#ifndef __ASSEMBLER__

#include <kernel/alloc/frame_allocator.h>
#include <kernel/core/result.h>
#include <kernel/core/type.h>

typedef uint64* pagetable_t;
typedef uint64 pte_t;
typedef uint64 pde_t;

typedef uint64 virt;
typedef uint64 phys;

bool virt_is_valid(virt virt);

void kvminit(void);
void kvminithart(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);

pagetable_t uvmcreate(void);
void uvmfirst(pagetable_t, uchar*, uint);
uint64 uvmalloc(pagetable_t, uint64, uint64, int);
uint64 uvmdealloc(pagetable_t, uint64, uint64);
rstatus_t uvmcopy(pagetable_t, pagetable_t, uint64);
void uvmfree(pagetable_t, uint64);
void uvmunmap(pagetable_t, uint64, uint64, int);
void uvmclear(pagetable_t, uint64);

/// Tries to handle page fault because of COW or Lazy-Init.
rstatus_t uvm_handle_page_fault(pagetable_t, virt);

/// Handles COW page fault. Prevents writting into COW page.
rstatus_t uvm_copy_on_write(pte_t* pte);

pte_t* vmwalk(pagetable_t, uint64, int);
uint64 vmwalkaddr(pagetable_t, uint64);
int vmmappages(pagetable_t, uint64, uint64, uint64, int);
rstatus_t vmcopyout(pagetable_t, uint64, char*, uint64);
int vmcopyin(pagetable_t, char*, uint64, uint64);
int vmcopyinstr(pagetable_t, char*, uint64, uint64);

/// @return a frame that is referenced by the `pte`.
frame pte_frame(pte_t pte);

/// Prints `pte` using printf. For debugging.
void pte_print(pte_t pte);

/// Prints `pagetable` using printf. For debugging.
void vm_print(pagetable_t pagetable);

#endif