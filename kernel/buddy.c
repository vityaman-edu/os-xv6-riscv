#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#include "buddy.h"
#include "list.h"

typedef struct list bd_list;

/// The allocator has `sz_info` for each size `k`.
/// Each `sz_info` has a `free` list, an array `alloc` to
/// keep track which blocks have been allocated, and
/// an `split` array to to keep track which blocks have
/// been split. The arrays are of type `byte` (which is
/// 1 byte), but the allocator uses 1 bit per block
/// (thus, one char records the info of 8 blocks).
typedef struct {
  bd_list free;
  byte* alloc;
  byte* split;
} sz_info;

/// The array array `bd_sizes` with size `bd_sizes_count`.
static int bd_sizes_count;
static sz_info* bd_sizes;

/// Start address of memory managed by the buddy allocator
static void* bd_base;

/// Global lock for the Buddy Allocator
static struct spinlock bd_lock;

// The smallest block size
#define LEAF_SIZE 16

// Largest index in bd_sizes array
#define MAXSIZE (bd_sizes_count - 1)

// Size of block at size k
#define BLK_SIZE(k) ((1 << (k)) * LEAF_SIZE)
#define HEAP_SIZE BLK_SIZE(MAXSIZE)

// Number of block at size k
#define NBLK(k) (1 << (MAXSIZE - (k)))

// Round up to the next multiple of sz
#define ROUNDUP(n, sz) (((((n)-1) / (sz)) + 1) * (sz))

// Return 1 if bit at position index in array is set to 1
int bit_isset(const byte* array, const int index) {
  const byte cell = array[index / 8];
  const byte mask = (1 << (index % 8));
  return (cell & mask) == mask;
}

// Set bit at position index in array to 1
void bit_set(byte* const array, const int index) {
  const byte cell = array[index / 8];
  const byte mask = (1 << (index % 8));
  array[index / 8] = (cell | mask);
}

// Clear bit at position index in array
void bit_clear(byte* const array, const int index) {
  const byte cell = array[index / 8];
  const byte mask = (1 << (index % 8));
  array[index / 8] = (cell & ~mask);
}

/// Print a bit vector as a list of ranges of 1 bits
void bd_print_vector(const byte* vector, const int len) {
  int last_bit = 1;
  int last_bit_index = 0;

  for (int bit_index = 0; bit_index < len; bit_index++) {
    if (last_bit == bit_isset(vector, bit_index)) {
      continue;
    }

    if (last_bit == 1) {
      printf(" [%d, %d)", last_bit_index, bit_index);
    }

    last_bit_index = bit_index;
    last_bit = bit_isset(vector, bit_index);
  }

  if (last_bit_index == 0 || last_bit == 1) {
    printf(" [%d, %d)", last_bit_index, len);
  }

  printf("\n");
}

void bd_print() {
  for (int k = 0; k < bd_sizes_count; k++) {
    printf("size %d (blksz %d nblk %d): free list: ", k, BLK_SIZE(k), NBLK(k));
    lst_print(&bd_sizes[k].free);
    printf("  alloc:");
    bd_print_vector(bd_sizes[k].alloc, NBLK(k));
    if (k > 0) {
      printf("  split:");
      bd_print_vector(bd_sizes[k].split, NBLK(k));
    }
  }
}

/// What is the first k such that 2^k >= n?
int min_power_of_two(const uint64 n) {
  int power_of_two = 0;
  uint64 size = LEAF_SIZE;
  while (size < n) {
    power_of_two++;
    size *= 2;
  }
  return power_of_two;
}

/// Compute the block index for address `addr` at size `k`
int blk_index(const int k, const char* addr) {
  int n = addr - (char*)bd_base;
  return n / BLK_SIZE(k);
}

/// Convert a block index at size `k` back into an address
void* blk_addr(const int k, const int block_index) {
  int n = block_index * BLK_SIZE(k);
  return (char*)bd_base + n;
}

/// Find a free block >= nbytes, starting with smallest
/// `power_of_two` possible.
/// Caller should hold `bd_lock`.
int bd_sizes_index_for(const uint64 nbytes) {
  for (                                            //
      int power_of_two = min_power_of_two(nbytes); //
      power_of_two < bd_sizes_count;
      power_of_two++ //
  ) {
    if (!lst_empty(&bd_sizes[power_of_two].free)) {
      return power_of_two;
    }
  }
  return bd_sizes_count;
}

void bd_mark_alloc(int size_pow, int block_index) {
  bit_set(bd_sizes[size_pow].alloc, block_index);
}

void bd_mark_split(int size_pow, int block_index) {
  bit_set(bd_sizes[size_pow].split, block_index);
}

/// Allocate `nbytes`, but malloc won't return anything
/// smaller than `LEAF_SIZE`
void* bd_malloc(const uint64 nbytes) {
  acquire(&bd_lock);

  const int min_index = min_power_of_two(nbytes);
  int index = bd_sizes_index_for(nbytes);

  if (index == bd_sizes_count) { // No free blocks
    release(&bd_lock);
    return nullptr;
  }

  // Found a block, pop it and potentially split it.
  char* const block = lst_pop(&bd_sizes[index].free);
  bd_mark_alloc(index, blk_index(index, block));

  for (; index > min_index; --index) {
    // split a block at size `index` and mark one half
    // allocated at size `index - 1` and put the buddy
    // on the free list at size `index - 1`

    // `block`'s buddy
    char* const buddy = block + BLK_SIZE(index - 1);

    bd_mark_split(index, blk_index(index, block));
    bd_mark_alloc(index - 1, blk_index(index - 1, block));

    lst_push(&bd_sizes[index - 1].free, buddy);
  }

  release(&bd_lock);
  return block;
}

// Find the size of the block that `addr` points to.
int blk_size(const char* addr) {
  for (int k = 0; k < bd_sizes_count; k++) {
    if (bit_isset(bd_sizes[k + 1].split, blk_index(k + 1, addr))) {
      return k;
    }
  }
  return 0;
}

/// Free memory pointed to by `addr`, which was earlier
/// allocated using `bd_malloc`.
void bd_free(void* addr) {
  acquire(&bd_lock);

  int size_pow = 0;
  for (size_pow = blk_size(addr); size_pow < MAXSIZE; ++size_pow) {
    int block_index = blk_index(size_pow, addr);
    int buddy_index =          //
        (block_index % 2 == 0) //
            ? block_index + 1
            : block_index - 1;

    // Free `block` at size `size_pow`
    bit_clear(bd_sizes[size_pow].alloc, block_index);
    if (bit_isset(bd_sizes[size_pow].alloc, buddy_index)) {
      break; // buddy is allocated, break out of loop
    }

    // Buddy is free, merge with buddy
    void* const buddy_addr = blk_addr(size_pow, buddy_index);
    lst_remove(buddy_addr); // Remove buddy from freelist
    if (buddy_index % 2 == 0) {
      addr = buddy_addr;
    }
    // At size `k + 1`, mark that the merged
    // buddy pair isn't `split` anymore
    bit_clear(bd_sizes[size_pow + 1].split, blk_index(size_pow + 1, addr));
  }

  lst_push(&bd_sizes[size_pow].free, addr);

  release(&bd_lock);
}

// Compute the first block at size `size_pow` that
// doesn't contain `block`
int blk_index_next(int size_pow, const char* block) {
  int index = (block - (char*)bd_base) / BLK_SIZE(size_pow);
  if ((block - (char*)bd_base) % BLK_SIZE(size_pow) != 0) {
    index++;
  }
  return index;
}

int log2(uint64 n) {
  int k = 0;
  while (n > 1) {
    k++;
    n = n >> 1;
  }
  return k;
}

bool bd_is_aligned(void* addr) {
  return (uint64)addr % LEAF_SIZE == 0;
}

// Mark memory from [start, stop), starting at size 0, as allocated.
void bd_mark(void* start, void* stop) {
  if (!bd_is_aligned(start) || !bd_is_aligned(stop)) {
    panic("bd_mark");
  }

  for (int size_pow = 0; size_pow < bd_sizes_count; size_pow++) {
    int next = blk_index_next(size_pow, stop);
    for (int i = blk_index(size_pow, start); i < next; ++i) {
      // if a block is allocated at size k, mark it as split too.
      if (size_pow > 0) {
        bit_set(bd_sizes[size_pow].split, i);
      }
      bit_set(bd_sizes[size_pow].alloc, i);
    }
  }
}

/// If a block is marked as allocated and the buddy is free,
/// put the buddy on the free list at size k.
int bd_initfree_pair(int size_pow, int block_index) {
  const int buddy_index = //
      (block_index % 2 == 0) ? block_index + 1 : block_index - 1;

  int free = 0;

  const bool is_block_allocated
      = bit_isset(bd_sizes[size_pow].alloc, block_index);
  const bool is_buddy_allocated
      = bit_isset(bd_sizes[size_pow].alloc, buddy_index);

  if (is_block_allocated != is_buddy_allocated) {
    // one of the pair is free
    free = BLK_SIZE(size_pow);

    if (bit_isset(bd_sizes[size_pow].alloc, block_index)) {
      // put buddy on free list
      lst_push(&bd_sizes[size_pow].free, blk_addr(size_pow, buddy_index));
    } else {
      // put bi on free list
      lst_push(&bd_sizes[size_pow].free, blk_addr(size_pow, block_index));
    }
  }

  return free;
}

// Initialize the free lists for each size k.
// For each size k, there are only two pairs that
// may have a buddy that should be on free list:
// bd_left and bd_right.
int bd_initfree(void* bd_left, void* bd_right) {
  int free = 0;

  for (int k = 0; k < MAXSIZE; k++) { // skip max size
    int left = blk_index_next(k, bd_left);
    int right = blk_index(k, bd_right);

    free += bd_initfree_pair(k, left);

    if (right <= left) {
      continue;
    }

    free += bd_initfree_pair(k, right);
  }

  return free;
}

// Mark the range `[bd_base, end)` as allocated
int bd_mark_data_structures(byte* end) {
  int meta = (end - (byte*)bd_base);

  printf(
      "bd: %d meta bytes for managing %d bytes of memory\n",
      meta,
      BLK_SIZE(MAXSIZE)
  );

  bd_mark(bd_base, end);

  return meta;
}

// Mark the range [end, HEAPSIZE) as allocated
int bd_mark_unavailable(void* end, void* left) {
  int unavailable = BLK_SIZE(MAXSIZE) - (end - bd_base);
  if (unavailable > 0) {
    unavailable = ROUNDUP(unavailable, LEAF_SIZE);
  }

  printf("bd: 0x%x bytes unavailable\n", unavailable);

  void* bd_end = bd_base + BLK_SIZE(MAXSIZE) - unavailable;

  bd_mark(bd_end, bd_base + BLK_SIZE(MAXSIZE));

  return unavailable;
}

// Initialize the buddy allocator: it manages
// memory from [base, end).
void bd_init(void* base, void* end) {
  byte* start = (byte*)ROUNDUP((uint64)base, LEAF_SIZE);

  initlock(&bd_lock, "buddy");

  bd_base = (void*)start;

  // compute the number of sizes we need to manage [start, end)
  bd_sizes_count = log2(((byte*)end - start) / LEAF_SIZE) + 1;
  if ((byte*)end - start > BLK_SIZE(MAXSIZE)) {
    bd_sizes_count++; // round up to the next power of 2
  }

  printf(
      "bd: memory sz is %d bytes; allocate an size array of length %d\n",
      (byte*)end - start,
      bd_sizes_count
  );

  // allocate bd_sizes array
  bd_sizes = (sz_info*)start;
  start += sizeof(sz_info) * bd_sizes_count;
  memset(bd_sizes, 0, sizeof(sz_info) * bd_sizes_count);

  // initialize free list and allocate the alloc 
  // array for each size k
  for (int k = 0; k < bd_sizes_count; k++) {
    lst_init(&bd_sizes[k].free);
    const int size = sizeof(char) * ROUNDUP(NBLK(k), 8) / 8;

    bd_sizes[k].alloc = start;
    memset(bd_sizes[k].alloc, 0, size);

    start += size;
  }

  // allocate the split array for each size k, 
  // except for k = 0, since we will not split 
  // blocks of size k = 0, the smallest size.
  for (int k = 1; k < bd_sizes_count; k++) {
    const int size = sizeof(char) * (ROUNDUP(NBLK(k), 8)) / 8;

    bd_sizes[k].split = start;
    memset(bd_sizes[k].split, 0, size);
    
    start += size;
  }

  start = (byte*)ROUNDUP((uint64)start, LEAF_SIZE);

  // done allocating; mark the memory range [base, p) as allocated, so
  // that buddy will not hand out that memory.
  int meta = bd_mark_data_structures(start);

  // mark the unavailable memory range [end, HEAP_SIZE) as allocated,
  // so that buddy will not hand out that memory.
  int unavailable = bd_mark_unavailable(end, start);
  void* bd_end = bd_base + BLK_SIZE(MAXSIZE) - unavailable;

  // initialize free lists for each size k
  int free = bd_initfree(start, bd_end);

  // check if the amount that is free is what we expect
  if (free != BLK_SIZE(MAXSIZE) - meta - unavailable) {
    printf("free %d %d\n", free, BLK_SIZE(MAXSIZE) - meta - unavailable);
    panic("bd_init: free mem");
  }
}
