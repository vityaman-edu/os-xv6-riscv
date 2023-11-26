// Mutual exclusion lock.
struct spinlock {
  UInt32 locked;       // Is the lock held?

  // For debugging:
  const char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};
