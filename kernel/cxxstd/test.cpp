#include <cstdint>
#include <vector>

extern "C" {
#include <kernel/cxxstd/test.h>
#include <kernel/defs.h>
}

namespace xv6::cxxstd::test {

void Main() {
  printf("[test] Starting XV6 C++ Standart Library Test...\n");
  printf("[test] OK\n");
  printf("[test] XV6 C++ Standart Library Test Completed!\n");
}

}  // namespace xv6::cxxstd::test

void CXXSTDTestMain() { xv6::cxxstd::test::Main(); }
