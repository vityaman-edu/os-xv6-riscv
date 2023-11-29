#pragma once

namespace xv6::kernel::library::error {

void Panic(const char* message) __attribute__((noreturn));

}  // namespace xv6::kernel::library::error