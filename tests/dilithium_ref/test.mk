include $(TESTS_DIR)/lib/pqc/dilithium.mk

# Iterations of the checks in test_dilithium.c (upstream default: 10000)
NTESTS ?= 1
TEST_CFLAGS += -DNTESTS=$(NTESTS)

# Separate build directory per configuration: build/SYSTEM/dilithium_ref-mode2-n1/
VARIANT := mode$(DILITHIUM_MODE)-n$(NTESTS)

# One iteration: keypair, sign and two verifications (~75M cycles for
# Dilithium5); signing varies (rejection sampling)
MAX_CYCLES := $(shell echo $$(( 250000000 * $(NTESTS) )))

# test_dilithium.c keeps ~16 KiB more on the stack than dilithium_cycles
ifeq ($(SYSTEM)-$(DILITHIUM_MODE),demo-3)
STACK_SIZE := 0x18000
endif
