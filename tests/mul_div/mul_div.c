// Checks M extension results and shows how to time code with mcycle.

#include "test_utils.h"

// volatile so the compiler cannot fold the operations at compile time
static volatile int32_t a = -123456, b = 789;

int main(void) {
  uint32_t start = read_cycles();

  CHECK_EQ(a * b, -97406784);
  CHECK_EQ(a / b, -156);
  CHECK_EQ(a % b, -372);
  CHECK_EQ((uint32_t)a / (uint32_t)b, 5443401);
  CHECK_EQ(((int64_t)a * b) >> 32, -1);  // mulh

  uint32_t cycles = read_cycles() - start;
  puts("cycles: 0x");
  puthex(cycles);
  putchar('\n');

  test_pass();
  return 0;
}
