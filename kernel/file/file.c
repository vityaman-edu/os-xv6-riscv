/// Support functions for system calls that
/// involve file descriptors.

#include "kernel/core/type.h"
#include "kernel/hw/arch/riscv/riscv.h"
#include "kernel/defs.h"
#include "kernel/core/param.h"
#include "kernel/file/fs.h"
#include "kernel/sync/spinlock.h"
#include "kernel/sync/sleeplock.h"
#include "kernel/file/stat.h"
#include "kernel/process/proc.h"

#include "kernel/alloc/buddy.h"
#include "kernel/file/file.h"

struct devsw devsw[NDEV];

void fileinit(void) {
  // Do nothing
}

/// Allocate a file structure.
struct file* filealloc(void) {
  struct file* file = buddy_malloc(sizeof(struct file));

  file->type = FD_NONE;
  file->ref = 1;
  file->readable = false;
  file->writable = false;
  file->pipe = nullptr;
  file->ip = nullptr;
  file->off = 0;
  file->major = 0;

  initlock(&file->lock, "file-lock");

  return file;
}

/// Increment ref count for file f.
struct file* filedup(struct file* file) {
  acquire(&file->lock);

  if (file->ref < 1) {
    panic("filedup on a free file");
  }

  file->ref += 1;

  release(&file->lock);
  return file;
}

/// Close file. (Decrement ref count, close when reaches 0.)
void fileclose(struct file* file) {
  acquire(&file->lock);

  if (file->ref < 1) {
    panic("fileclose on a free file");
  }

  file->ref -= 1;
  const bool is_referenced = 0 < file->ref;

  release(&file->lock);

  if (is_referenced) {
    return;
  }

  if (file->type == FD_PIPE) {
    pipeclose(file->pipe, file->writable);
  } else if (file->type == FD_INODE || file->type == FD_DEVICE) {
    begin_op();
    iput(file->ip);
    end_op();
  }

  buddy_free(file);
}

/// Get metadata about file f.
/// addr is a user virtual address, pointing to a struct stat.
int filestat(struct file* file, uint64 addr) {
  const struct proc* proc = myproc();

  if (file->type == FD_INODE || file->type == FD_DEVICE) {
    ilock(file->ip);

    struct stat stat;
    stati(file->ip, &stat);

    iunlock(file->ip);

    int status = vmcopyout(proc->pagetable, addr, (char*)&stat, sizeof(stat));
    if (status < 0) {
      return -1;
    }

    return 0;
  }

  return -1;
}

/// Read from file f.
/// addr is a user virtual address.
int fileread(struct file* file, uint64 addr, int n) {
  if (!file->readable) {
    return -1;
  }

  if (file->type == FD_PIPE) {
    return piperead(file->pipe, addr, n);
  }

  if (file->type == FD_DEVICE) {
    if (file->major < 0 || NDEV <= file->major) {
      return -1;
    }

    if (!devsw[file->major].read) {
      return -1;
    }

    return devsw[file->major].read(1, addr, n);
  }

  if (file->type == FD_INODE) {
    ilock(file->ip);

    const int count = readi(file->ip, 1, addr, file->off, n);
    if (count > 0) {
      file->off += count;
    }

    iunlock(file->ip);

    return count;
  }

  panic("fileread failed");
  return 0;
}

/// Write to file `file`.
/// `addr` is a user virtual address.
int filewrite(struct file* file, uint64 addr, int n) {
  if (!file->writable) {
    return -1;
  }

  if (file->type == FD_PIPE) {
    return pipewrite(file->pipe, addr, n);
  }

  if (file->type == FD_DEVICE) {
    if (file->major < 0 || NDEV <= file->major) {
      return -1;
    }

    if (!devsw[file->major].write) {
      return -1;
    }

    return devsw[file->major].write(1, addr, n);
  }

  if (file->type == FD_INODE) {
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    const int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * BSIZE;

    int index = 0;
    while (index < n) {
      int remaining = n - index;
      if (remaining > max) {
        remaining = max;
      }

      begin_op();
      ilock(file->ip);

      const int count = writei(file->ip, 1, addr + index, file->off, remaining);
      if (count > 0) {
        file->off += count;
      }

      iunlock(file->ip);
      end_op();

      if (count != remaining) {
        break; // error from writei
      }

      index += count;
    }

    return (index == n ? n : -1);
  }

  panic("filewrite");
  return 0;
}
