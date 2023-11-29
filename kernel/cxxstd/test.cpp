#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" {
#include <kernel/cxxstd/test.h>
#include <kernel/legacy/defs.h>
}

namespace xv6::cxxstd::test {

void Main() {
  printf("[test] Starting XV6 C++ Standart Library Test...\n");

  printf("[test] Testing 'std::sort'...\n");
  auto array = std::array<int, 6>({6, 5, 4, 3, 2, 1});
  printf("before: ");
  for (auto element : array) {
    printf("%d ", element);
  }
  printf("\n");

  std::sort(array.begin(), array.end());

  printf("after: ");
  for (auto element : array) {
    printf("%d ", element);
  }
  printf("\n");

  printf("[test] XV6 C++ Standart Library Test Completed!\n");
}

}  // namespace xv6::cxxstd::test

void CXXSTDTestMain() {
  xv6::cxxstd::test::Main();
}
