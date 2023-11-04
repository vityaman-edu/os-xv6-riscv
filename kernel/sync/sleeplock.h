#ifndef XV6_KERNEL_SLEEPLOCK_H
#define XV6_KERNEL_SLEEPLOCK_H

#include "../core/type.h"
#include "spinlock.h"

/// Long-term locks for processes
struct sleeplock {
  uint locked;        // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock

  // For debugging:
  char* name; // Name of lock.
  int pid;    // Process holding lock
};

void initsleeplock(struct sleeplock* lock, char* name);

void acquiresleep(struct sleeplock* lock);

void releasesleep(struct sleeplock* lock);

int holdingsleep(struct sleeplock* lock);

#endif // XV6_KERNEL_SLEEPLOCK_H
