// REQUIRES: gwp_asan
// RUN: %clangxx_gwp_asan %s -o %t
// RUN: not %run %t 2>&1 | FileCheck %s

// CHECK: GWP-ASan detected a memory error
// CHECK: Invalid (wild) free at 0x{{[a-f0-9]+}} (1 byte to the right of a
// CHECK-SAME: 1-byte allocation

#include <cstdlib>

int main() {
  char *Ptr =
      reinterpret_cast<char *>(malloc(1));
  free(Ptr + 1);
  return 0;
}
