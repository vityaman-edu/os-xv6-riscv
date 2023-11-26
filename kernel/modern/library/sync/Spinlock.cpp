#include <kernel/modern/library/sync/Spinlock.hpp>

namespace xv6::kernel::library::sync {

Spinlock::Spinlock(const char* name) {
  initlock(&lock_, name);
}

SpinlockGuard Spinlock::lock() {
  acquire();
  return SpinlockGuard(this);
}

void Spinlock::acquire() {
  using ::acquire;
  acquire(&lock_);
}

void Spinlock::release() {
  using ::release;
  release(&lock_);
}

SpinlockGuard::SpinlockGuard(Spinlock* origin) : origin_(origin) {
}

SpinlockGuard::SpinlockGuard(SpinlockGuard&& other)
    : SpinlockGuard(other.origin_) {
  other.origin_ = nullptr;
}

SpinlockGuard::~SpinlockGuard() {
  if (origin_ != nullptr) {
    origin_->release();
  }
}

}  // namespace xv6::kernel::library::sync