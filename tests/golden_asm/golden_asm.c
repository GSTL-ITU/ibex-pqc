// Example of a golden-output test (see test.mk) with an assembly source.

#include "test_utils.h"

uint32_t sum_to(uint32_t n);  // sum.S

int main(void) {
  for (uint32_t n = 1; n <= 4; n++) {
    puthex(sum_to(n * 10));
    putchar('\n');
  }
  return 0;
}
