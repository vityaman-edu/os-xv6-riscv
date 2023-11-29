#pragma once

#include <kernel/core/type.h>

/// Mutual exclusion lock.
struct spinlock {
  uint locked; // Is the lock held?

  // For debugging:
  char* name;      // Name of lock.
  struct cpu* cpu; // The cpu holding the lock.
};

/// Init lock
void initlock(struct spinlock* lock, char* name);

/// Acquire the lock.
/// Loops (spins) until the lock is acquired.
void acquire(struct spinlock* lock);

/// Release the lock.
void release(struct spinlock* lock);
