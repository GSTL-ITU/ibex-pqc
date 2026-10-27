#include "test_utils.h"

int main(void) {
  puts("Hello from Ibex!\n");
  puthex(0xDEADBEEF);
  putchar('\n');

  test_pass();
  return 0;
}
