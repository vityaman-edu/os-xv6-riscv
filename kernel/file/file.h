#pragma once

#include <kernel/core/type.h>
#include <kernel/file/fs.h>
#include <kernel/sync/sleeplock.h>
#include <kernel/sync/spinlock.h>

struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;

  /// reference count
  int ref;

  struct spinlock lock;

  bool readable;
  bool writable;
  struct pipe* pipe; // FD_PIPE
  struct inode* ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE
  short major;       // FD_DEVICE
};

#define major(dev) ((dev) >> 16 & 0xFFFF)
#define minor(dev) ((dev)&0xFFFF)
#define mkdev(m, n) ((uint)((m) << 16 | (n)))

// in-memory copy of an inode
struct inode {
  uint dev;              // Device number
  uint inum;             // Inode number
  int ref;               // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;             // inode has been read from disk?

  short type; // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT + 1];
};

// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
