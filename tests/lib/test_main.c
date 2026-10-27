// The startup code (crt0.S of each system) calls main() and then halts the
// simulation, ignoring main's return value. The Makefile links with
// -Wl,--wrap=main, so crt0's call lands here instead: the real main() runs and
// its return value becomes the verdict, 0 = PASS. Tests that call test_pass()
// or test_fail() halt inside main() and never get back here.
//
// The stack is painted before main() (see stack.h), so a stack overflow fails
// the test even if the test doesn't check for it. With -DTEST_REPORT_STACK
// (set by the PQC tests), the peak stack usage is printed too.

#include "stack.h"
#include "test_utils.h"

int __real_main(void);

static void put_dec(uint32_t v) {
  char buf[11];
  int i = sizeof(buf);
  buf[--i] = '\0';
  do {
    buf[--i] = '0' + v % 10;
    v /= 10;
  } while (v);
  puts(&buf[i]);
}

int __wrap_main(void) {
  stack_paint();
  int ret = __real_main();

#ifdef TEST_REPORT_STACK
  puts("peak stack usage (whole test): ");
  put_dec(stack_max_used());
  puts(" of ");
  put_dec(stack_size());
  puts(" bytes\n");
#endif
  if (stack_overflowed()) {
    puts("stack overflow: raise STACK_SIZE (in test.mk)\n");
    test_fail();
  }
  if (ret != 0) {
    puts("main returned 0x");
    puthex((uint32_t)ret);
    putchar('\n');
    test_fail();
  }
  test_pass();
  return ret;
}
