#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#include "buddy.h"
#include "list.h"
#include "bits.h"

// The smallest block size
#define LEAF_SIZE 16

// Largest index in bd_sizes array
#define MAXSIZE (buddy_size_groups_count - 1)

// Size of block at size k
#define BLK_SIZE(k) ((1L << (k)) * LEAF_SIZE)
#define HEAP_SIZE BLK_SIZE(MAXSIZE)

// Number of block at size k
#define NBLK(k) (1 << (MAXSIZE - (k)))

// Round up to the next multiple of sz
#define ROUNDUP(n, sz) (((((n)-1) / (sz)) + 1) * (sz))

// The allocator has sz_info for each size k. Each sz_info has a free
// list, an array alloc to keep track which blocks have been
// allocated, and an split array to to keep track which blocks have
// been split.  The arrays are of type char (which is 1 byte), but the
// allocator uses 1 bit per block (thus, one char records the info of
// 8 blocks).
typedef struct {
  struct list freelist;
  bits pair_alloc_xor;
  bits split;
} buddy_size_group_info;

// The array of `buddy_size_info`
static buddy_size_group_info* buddy_size_groups;
static int buddy_size_groups_count;

// Start address of memory managed by the buddy allocator
static void* buddy_base;

// Lock
static struct spinlock buddy_lock;

int buddy_pair_index(int block_index) {
  if (block_index % 2 == 1) {
    block_index -= 1;
  }
  return block_index / 2;
}

// Print buddy's data structures
void buddy_print() {
  for (int k = 0; k < buddy_size_groups_count; k++) {
    printf("size %d (blksz %d nblk %d): free list: ", k, BLK_SIZE(k), NBLK(k));
    lst_print(&buddy_size_groups[k].freelist);
    printf("  pair_xor_alloc:");
    bits_print((fat_bits){buddy_size_groups[k].pair_alloc_xor, NBLK(k) / 2});
    if (k > 0) {
      printf("  split:");
      bits_print((fat_bits){buddy_size_groups[k].split, NBLK(k)});
    }
  }
}

// What is the first k such that 2^k >= n?
int firstk(uint64 n) {
  int k = 0;
  uint64 size = LEAF_SIZE;
  while (size < n) {
    k++;
    size *= 2;
  }
  return k;
}

// Compute the block index for address p at size k
int blk_index(int k, char* p) {
  int n = p - (char*)buddy_base;
  return n / BLK_SIZE(k);
}

// Convert a block index at size k back into an address
void* addr(int k, int bi) {
  int n = bi * BLK_SIZE(k);
  return (char*)buddy_base + n;
}

// allocate nbytes, but malloc won't return anything smaller than LEAF_SIZE
void* buddy_malloc(uint64 nbytes) {
  int fk, k;

  acquire(&buddy_lock);

  // Find a free block >= nbytes, starting with smallest k possible
  fk = firstk(nbytes);
  for (k = fk; k < buddy_size_groups_count; k++) {
    if (!lst_empty(&buddy_size_groups[k].freelist))
      break;
  }
  if (k >= buddy_size_groups_count) { // No free blocks?
    release(&buddy_lock);
    return 0;
  }

  // Found a block; pop it and potentially split it.
  char* p = lst_pop(&buddy_size_groups[k].freelist);
  bits_switch(buddy_size_groups[k].pair_alloc_xor, buddy_pair_index(blk_index(k, p)));
  for (; k > fk; k--) {
    // split a block at size k and mark one half allocated at size k-1
    // and put the buddy on the free list at size k-1
    char* q = p + BLK_SIZE(k - 1); // p's buddy
    bits_set(buddy_size_groups[k].split, blk_index(k, p));
    bits_switch(
        buddy_size_groups[k - 1].pair_alloc_xor, buddy_pair_index(blk_index(k - 1, p))
    );
    lst_push(&buddy_size_groups[k - 1].freelist, q);
  }
  release(&buddy_lock);

  return p;
}

// Find the size of the block that p points to.
int size(char* p) {
  for (int k = 0; k < buddy_size_groups_count; k++) {
    if (bits_is_set(buddy_size_groups[k + 1].split, blk_index(k + 1, p))) {
      return k;
    }
  }
  return 0;
}

// Free memory pointed to by p, which was earlier allocated using
// bd_malloc.
void buddy_free(void* p) {
  void* q;
  int k;

  acquire(&buddy_lock);
  for (k = size(p); k < MAXSIZE; k++) {
    int bi = blk_index(k, p);
    int pi = buddy_pair_index(bi);
    int buddy = (bi % 2 == 0) ? bi + 1 : bi - 1;
    bits_switch(buddy_size_groups[k].pair_alloc_xor, pi);
    if (bits_is_set(buddy_size_groups[k].pair_alloc_xor, pi)) {
      // buddy is allocated, break out of loop
      break;
    }
    // budy is free; merge with buddy
    q = addr(k, buddy);
    lst_remove(q); // remove buddy from free list
    if (buddy % 2 == 0) {
      p = q;
    }
    // at size k+1, mark that the merged buddy pair isn't split
    // anymore
    bits_clear(buddy_size_groups[k + 1].split, blk_index(k + 1, p));
  }
  lst_push(&buddy_size_groups[k].freelist, p);
  release(&buddy_lock);
}

// Compute the first block at size k that doesn't contain p
int blk_index_next(int k, char* p) {
  int n = (p - (char*)buddy_base) / BLK_SIZE(k);
  if ((p - (char*)buddy_base) % BLK_SIZE(k) != 0)
    n++;
  return n;
}

int log2(uint64 n) {
  int k = 0;
  while (n > 1) {
    k++;
    n = n >> 1;
  }
  return k;
}

// Mark memory from [start, stop), starting at size 0, as allocated.
void bd_mark(void* start, void* stop) {
  int bi, bj;

  if (((uint64)start % LEAF_SIZE != 0) || ((uint64)stop % LEAF_SIZE != 0))
    panic("bd_mark");

  for (int k = 0; k < buddy_size_groups_count; k++) {
    bi = blk_index(k, start);
    bj = blk_index_next(k, stop);
    for (; bi < bj; bi++) {
      if (k > 0) {
        // if a block is allocated at size k, mark it as split too.
        bits_set(buddy_size_groups[k].split, bi);
      }
      bits_switch(buddy_size_groups[k].pair_alloc_xor, buddy_pair_index(bi));
    }
  }
}

int bd_initfree_pair_right(int k, int bi) {
  int buddy = (bi % 2 == 0) ? bi + 1 : bi - 1;
  int free = 0;
  if (bits_is_set(buddy_size_groups[k].pair_alloc_xor, buddy_pair_index(bi))) {
    // one of the pair is free
    free = BLK_SIZE(k);
    lst_push(
        &buddy_size_groups[k].freelist, addr(k, buddy)
    ); // put buddy on free list
  }
  return free;
}

// If a block is marked as allocated and the buddy is free, put the
// buddy on the free list at size k.
int bd_initfree_pair_left(int k, int bi) {
  int free = 0;
  if (bits_is_set(buddy_size_groups[k].pair_alloc_xor, buddy_pair_index(bi))) {
    // one of the pair is free
    free = BLK_SIZE(k);
    lst_push(&buddy_size_groups[k].freelist, addr(k, bi)); // put bi on free list
  }
  return free;
}

// Initialize the free lists for each size k.  For each size k, there
// are only two pairs that may have a buddy that should be on free list:
// bd_left and bd_right.
int bd_initfree(void* bd_left, void* bd_right) {
  int free = 0;

  for (int k = 0; k < MAXSIZE; k++) { // skip max size
    int left = blk_index_next(k, bd_left);
    int right = blk_index(k, bd_right);
    free += bd_initfree_pair_left(k, left);
    if (right <= left)
      continue;
    free += bd_initfree_pair_right(k, right);
  }
  return free;
}

// Mark the range [bd_base,p) as allocated
int bd_mark_data_structures(byte* p) {
  int meta = p - (byte*)buddy_base;
  printf(
      "bd: %d meta bytes for managing %d bytes of memory\n",
      meta,
      BLK_SIZE(MAXSIZE)
  );
  bd_mark(buddy_base, p);
  return meta;
}

// Mark the range [end, HEAPSIZE) as allocated
int bd_mark_unavailable(void* end, void* left) {
  int unavailable = BLK_SIZE(MAXSIZE) - (end - buddy_base);
  if (unavailable > 0)
    unavailable = ROUNDUP(unavailable, LEAF_SIZE);
  printf("bd: 0x%x bytes unavailable\n", unavailable);

  void* bd_end = buddy_base + BLK_SIZE(MAXSIZE) - unavailable;
  bd_mark(bd_end, buddy_base + BLK_SIZE(MAXSIZE));
  return unavailable;
}

// Initialize the buddy allocator: it manages memory from [base, end).
void buddy_init(void* base, void* end) {
  byte* p = (byte*)ROUNDUP((uint64)base, LEAF_SIZE);
  int sz;

  initlock(&buddy_lock, "buddy");
  buddy_base = (void*)p;

  // compute the number of sizes we need to manage [base, end)
  buddy_size_groups_count = log2(((byte*)end - p) / LEAF_SIZE) + 1;
  if ((byte*)end - p > BLK_SIZE(MAXSIZE)) {
    buddy_size_groups_count++; // round up to the next power of 2
  }

  printf(
      "bd: memory sz is %d bytes; allocate an size array of length %d\n",
      (byte*)end - p,
      buddy_size_groups_count
  );

  // allocate bd_sizes array
  buddy_size_groups = (buddy_size_group_info*)p;
  p += sizeof(buddy_size_group_info) * buddy_size_groups_count;
  memset(buddy_size_groups, 0, sizeof(buddy_size_group_info) * buddy_size_groups_count);

  // initialize free list and allocate the alloc array for each size k
  for (int k = 0; k < buddy_size_groups_count; k++) {
    lst_init(&buddy_size_groups[k].freelist);
    sz = sizeof(char) * ROUNDUP(NBLK(k), 8) / 8;
    sz = sz / 2 > 0 ? sz / 2 : 1;
    buddy_size_groups[k].pair_alloc_xor.as_bytes = p;
    memset(buddy_size_groups[k].pair_alloc_xor.as_bytes, 0, sz);
    p += sz;
  }

  // allocate the split array for each size k, except for k = 0, since
  // we will not split blocks of size k = 0, the smallest size.
  for (int k = 1; k < buddy_size_groups_count; k++) {
    sz = sizeof(char) * (ROUNDUP(NBLK(k), 8)) / 8;
    buddy_size_groups[k].split.as_bytes = p;
    memset(buddy_size_groups[k].split.as_bytes, 0, sz);
    p += sz;
  }
  p = (byte*)ROUNDUP((uint64)p, LEAF_SIZE);

  // done allocating; mark the memory range [base, p) as allocated, so
  // that buddy will not hand out that memory.
  int meta = bd_mark_data_structures(p);

  // mark the unavailable memory range [end, HEAP_SIZE) as allocated,
  // so that buddy will not hand out that memory.
  int unavailable = bd_mark_unavailable(end, p);
  void* bd_end = buddy_base + BLK_SIZE(MAXSIZE) - unavailable;

  // initialize free lists for each size k
  int free = bd_initfree(p, bd_end);

  // check if the amount that is free is what we expect
  if (free != BLK_SIZE(MAXSIZE) - meta - unavailable) {
    printf("free %d %d\n", free, BLK_SIZE(MAXSIZE) - meta - unavailable);
    panic("bd_init: free mem");
  }
}