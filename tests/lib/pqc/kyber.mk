# Kyber (ML-KEM) reference implementation, shared by the kyber_* tests.
# Include it from a test.mk:  include $(TESTS_DIR)/lib/pqc/kyber.mk
#
# The sources are compiled straight from the kyber submodule (ref/), except
# ref/randombytes.c, which needs an OS. lib/pqc/randombytes.c replaces it: a
# fixed-seed PRNG, so runs and cycle counts are reproducible (NOT secure).

# Parameter set: 2 = Kyber512, 3 = Kyber768, 4 = Kyber1024
KYBER_K ?= 3

KYBER_REF := $(TOP_DIR)/kyber/ref
ifeq ($(wildcard $(KYBER_REF)/kem.c),)
$(error $(KYBER_REF) is missing, run: git submodule update --init kyber)
endif

EXTRA_SRCS  += $(filter-out $(KYBER_REF)/randombytes.c,$(wildcard $(KYBER_REF)/*.c)) \
	$(TESTS_DIR)/lib/pqc/randombytes.c
EXTRA_INCS  += $(KYBER_REF) $(TESTS_DIR)/lib/pqc
TEST_CFLAGS += -DTEST_REPORT_STACK -DKYBER_K=$(KYBER_K)

# Keys, ciphertexts and polynomial vectors live on the stack
# (peak: ~10 KiB Kyber512, ~15 KiB Kyber768, ~21 KiB Kyber1024)
STACK_SIZE := 0x8000
# Same as the pulpissimo-pqc results, so the numbers are comparable
OPT := -O2
# No instruction trace (~0.5 GB per run otherwise); TRACE=1 turns it on
TRACE := 0
