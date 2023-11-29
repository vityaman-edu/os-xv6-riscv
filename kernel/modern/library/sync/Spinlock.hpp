#pragma once

extern "C" {
#include <kernel/legacy/defs.h>
#include <kernel/legacy/spinlock.h>
}

namespace xv6::kernel::library::sync {

class SpinlockGuard;

class Spinlock {
 public:
  explicit Spinlock(const char* name);

  SpinlockGuard lock();

  Spinlock(const Spinlock&) = delete;
  Spinlock& operator=(const Spinlock&) = delete;
  Spinlock(Spinlock&&) = delete;
  Spinlock& operator=(Spinlock&&) = delete;
  ~Spinlock() = default;

 private:
  void acquire();
  void release();

  struct spinlock lock_;

  friend SpinlockGuard;
};

class SpinlockGuard {
 public:
  SpinlockGuard(SpinlockGuard&&);
  ~SpinlockGuard();

  SpinlockGuard(const SpinlockGuard&) = delete;
  SpinlockGuard& operator=(const SpinlockGuard&) = delete;
  SpinlockGuard& operator=(SpinlockGuard&&) = delete;

 private:
  explicit SpinlockGuard(Spinlock* origin);

  Spinlock* origin_ = nullptr;

  friend Spinlock;
};

}  // namespace xv6::kernel::library::sync