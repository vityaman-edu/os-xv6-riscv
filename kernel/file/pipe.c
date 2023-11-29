#include <kernel/core/param.h>
#include <kernel/core/type.h>
#include <kernel/defs.h>
#include <kernel/file/file.h>
#include <kernel/file/fs.h>
#include <kernel/hardware/riscv.h>
#include <kernel/memory/vm.h>
#include <kernel/process/proc.h>
#include <kernel/sync/sleeplock.h>
#include <kernel/sync/spinlock.h>

#define PIPESIZE 512

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;    // number of bytes read
  uint nwrite;   // number of bytes written
  int readopen;  // read fd is still open
  int writeopen; // write fd is still open
};

int pipealloc(struct file** rfd, struct file** wfd) {
  struct pipe* pipe = nullptr;
  *rfd = *wfd = nullptr;

  *rfd = filealloc();
  if (*rfd == nullptr) {
    goto bad;
  }

  *wfd = filealloc();
  if ((*wfd) == nullptr) {
    goto bad;
  }

  pipe = (struct pipe*)kalloc();
  if (pipe == nullptr) {
    goto bad;
  }

  pipe->readopen = 1;
  pipe->writeopen = 1;
  pipe->nwrite = 0;
  pipe->nread = 0;

  initlock(&pipe->lock, "pipe");

  (*rfd)->type = FD_PIPE;
  (*rfd)->readable = true;
  (*rfd)->writable = false;
  (*rfd)->pipe = pipe;

  (*wfd)->type = FD_PIPE;
  (*wfd)->readable = false;
  (*wfd)->writable = true;
  (*wfd)->pipe = pipe;

  return 0;

bad:
  if (pipe) {
    kfree((char*)pipe);
  }
  if (*rfd) {
    fileclose(*rfd);
  }
  if (*wfd) {
    fileclose(*wfd);
  }
  return -1;
}

void pipeclose(struct pipe* pi, int writable) {
  acquire(&pi->lock);
  if (writable) {
    pi->writeopen = 0;
    wakeup(&pi->nread);
  } else {
    pi->readopen = 0;
    wakeup(&pi->nwrite);
  }
  if (pi->readopen == 0 && pi->writeopen == 0) {
    release(&pi->lock);
    kfree((char*)pi);
  } else {
    release(&pi->lock);
  }
}

int pipewrite(struct pipe* pi, uint64 addr, int n) {
  int i = 0;
  struct proc* pr = myproc();

  acquire(&pi->lock);
  while (i < n) {
    if (pi->readopen == 0 || killed(pr)) {
      release(&pi->lock);
      return -1;
    }
    if (pi->nwrite == pi->nread + PIPESIZE) { // DOC: pipewrite-full
      wakeup(&pi->nread);
      sleep(&pi->nwrite, &pi->lock);
    } else {
      char ch;
      if (vmcopyin(pr->pagetable, &ch, addr + i, 1) == -1) {
        break;
      }
      pi->data[pi->nwrite++ % PIPESIZE] = ch;
      i++;
    }
  }
  wakeup(&pi->nread);
  release(&pi->lock);

  return i;
}

int piperead(struct pipe* pi, uint64 addr, int n) {
  int i;
  struct proc* pr = myproc();
  char ch;

  acquire(&pi->lock);
  while (pi->nread == pi->nwrite && pi->writeopen) { // DOC: pipe-empty
    if (killed(pr)) {
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->nread, &pi->lock); // DOC: piperead-sleep
  }
  for (i = 0; i < n; i++) { // DOC: piperead-copy
    if (pi->nread == pi->nwrite) {
      break;
    }
    ch = pi->data[pi->nread++ % PIPESIZE];
    if (vmcopyout(pr->pagetable, addr + i, &ch, 1) == -1) {
      break;
    }
  }
  wakeup(&pi->nwrite); // DOC: piperead-wakeup
  release(&pi->lock);
  return i;
}
