// Helpers for tests under tests/. The Makefile decides PASS/FAIL by looking
// for "TEST PASSED" / "TEST FAILED" in the program output.
//
// This includes the header of the system the test is built for (the Makefile
// defines TEST_SYSTEM_SIMPLE or TEST_SYSTEM_DEMO). Both provide putchar, puts,
// puthex and sim_halt. Include only this file for them: simple_system_common.h
// has no working include guard, so including it twice fails.

#ifndef TEST_UTILS_H__
#define TEST_UTILS_H__

#include <stdint.h>

#if defined(TEST_SYSTEM_SIMPLE)
#include "simple_system_common.h"
#elif defined(TEST_SYSTEM_DEMO)
#include "demo_system.h"
#else
#error "Build with TEST_SYSTEM_SIMPLE or TEST_SYSTEM_DEMO defined (tests/Makefile does this)"
#endif

static inline void test_pass(void) {
  puts("TEST PASSED\n");
  sim_halt();
}

static inline void test_fail(void) {
  puts("TEST FAILED\n");
  sim_halt();
}

#define TU_STR2(x) #x
#define TU_STR(x) TU_STR2(x)

// Stop the test with a failure if cond is false
#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      puts("CHECK failed: " #cond " (" __FILE__ ":" TU_STR(__LINE__) ")\n"); \
      test_fail();                                                     \
    }                                                                  \
  } while (0)

// Like CHECK, also prints both values in hex
#define CHECK_EQ(a, b)                                                 \
  do {                                                                 \
    uint32_t _a = (uint32_t)(a), _b = (uint32_t)(b);                   \
    if (_a != _b) {                                                    \
      puts("CHECK_EQ failed: " #a " == " #b " (" __FILE__ ":" TU_STR(__LINE__) ")\n  got 0x"); \
      puthex(_a);                                                      \
      puts(", expected 0x");                                           \
      puthex(_b);                                                      \
      putchar('\n');                                                   \
      test_fail();                                                     \
    }                                                                  \
  } while (0)

// Cycle counter (mcycle, lower 32 bits)
static inline uint32_t read_cycles(void) {
  uint32_t c;
  asm volatile("csrr %0, mcycle" : "=r"(c));
  return c;
}

#endif  // TEST_UTILS_H__
