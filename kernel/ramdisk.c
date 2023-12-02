/// ramdisk that uses the disk image loaded by qemu -initrd fs.img

#include <kernel/core/param.h>
#include <kernel/core/type.h>
#include <kernel/defs.h>
#include <kernel/file/buf.h>
#include <kernel/file/fs.h>
#include <kernel/hw/memlayout.h>
#include <kernel/hw/arch/riscv/register.h>
#include <kernel/sync/sleeplock.h>
#include <kernel/sync/spinlock.h>

void ramdiskinit() {
}

// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void ramdiskrw(struct buf* b) {
  if (!holdingsleep(&b->lock)) {
    panic("ramdiskrw: buf not locked");
  }
  if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID) {
    panic("ramdiskrw: nothing to do");
  }

  if (b->blockno >= FSSIZE) {
    panic("ramdiskrw: blockno too big");
  }

  uint64 diskaddr = b->blockno * BSIZE;
  char* addr = (char*)RAMDISK + diskaddr;

  if (b->flags & B_DIRTY) {
    // write
    memmove(addr, b->data, BSIZE);
    b->flags &= ~B_DIRTY;
  } else {
    // read
    memmove(b->data, addr, BSIZE);
    b->flags |= B_VALID;
  }
}