#include <cstddef>

extern "C" {
#include <kernel/legacy/defs.h>
}

/// Support C++ Standart Library Allocator

auto operator new(std::size_t size) -> void* {  // NOLINT
  return bd_malloc(size);
}

auto operator new[](std::size_t size) -> void* {  // NOLINT
  return bd_malloc(size);
}

auto operator delete(void* ptr) noexcept -> void {  // NOLINT
  bd_free(ptr);
}

auto operator delete[](void* ptr) noexcept -> void {  // NOLINT
  bd_free(ptr);
}

auto operator delete(void* ptr, std::size_t) noexcept -> void {  // NOLINT
  bd_free(ptr);
}

auto operator delete[](void* ptr, std::size_t) noexcept -> void {  // NOLINT
  bd_free(ptr);
}

auto operator new(std::size_t, void* ptr) throw() -> void* {  // NOLINT
  return ptr;
}

auto operator new[](std::size_t, void* ptr) throw() -> void* {  // NOLINT
  return ptr;
}

auto operator delete(void*, void*) throw() -> void {  // NOLINT
}

auto operator delete[](void*, void*) throw() -> void {  // NOLINT
}