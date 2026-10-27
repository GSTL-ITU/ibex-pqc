# Dilithium (ML-DSA) reference implementation, shared by the dilithium_* tests.
# Include it from a test.mk:  include $(TESTS_DIR)/lib/pqc/dilithium.mk
#
# The sources are compiled straight from the dilithium submodule (ref/),
# except ref/randombytes.c, which needs an OS. lib/pqc/randombytes.c replaces
# it, the same fixed-seed PRNG as for Kyber (NOT secure).

# Parameter set: 2, 3 or 5 (Dilithium2 / 3 / 5)
DILITHIUM_MODE ?= 2
ifeq ($(filter 2 3 5,$(DILITHIUM_MODE)),)
$(error DILITHIUM_MODE must be 2, 3 or 5)
endif

DILITHIUM_REF := $(TOP_DIR)/dilithium/ref
ifeq ($(wildcard $(DILITHIUM_REF)/sign.c),)
$(error $(DILITHIUM_REF) is missing, run: git submodule update --init dilithium)
endif

EXTRA_SRCS  += $(filter-out $(DILITHIUM_REF)/randombytes.c,$(wildcard $(DILITHIUM_REF)/*.c)) \
	$(TESTS_DIR)/lib/pqc/randombytes.c
EXTRA_INCS  += $(DILITHIUM_REF)
TEST_CFLAGS += -DTEST_REPORT_STACK -DDILITHIUM_MODE=$(DILITHIUM_MODE)

# The reference code keeps the public matrix and other polynomial vectors on
# the stack. Measured peak (whole test, printed by every run):
#
#   mode   dilithium_cycles   dilithium_ref
#   2      52,740 B           64,020 B
#   3      80,388 B           96,452 B
#   5     123,380 B          144,900 B
#
# Simple system (1 MiB RAM): the pulpissimo-pqc sizes, 128 KiB for Dilithium2
# and 192 KiB for Dilithium3/5.
# Demo system (128 KiB RAM for code, data and stack): as small as fits with
# some margin. Dilithium5 needs more stack than the whole RAM, so it runs on
# the simple system only. dilithium_ref/test.mk raises the Dilithium3 size.
# A stack overflow fails the test (lib/test_main.c).
ifeq ($(SYSTEM),demo)
ifeq ($(DILITHIUM_MODE),2)
STACK_SIZE := 0x12000
else
STACK_SIZE := 0x15000
endif
else
ifeq ($(DILITHIUM_MODE),2)
STACK_SIZE := 0x20000
else
STACK_SIZE := 0x30000
endif
endif

ifeq ($(DILITHIUM_MODE),5)
SYSTEMS := simple
endif

# Same as the pulpissimo-pqc results, so the numbers are comparable
OPT := -O2
# No instruction trace (several GB per run otherwise); TRACE=1 turns it on
TRACE := 0
