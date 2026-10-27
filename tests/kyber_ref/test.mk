include $(TESTS_DIR)/lib/pqc/kyber.mk

# Iterations of the three checks in test_kyber.c (upstream default: 1000)
NTESTS ?= 1
TEST_CFLAGS += -DNTESTS=$(NTESTS)

# Separate build directory per configuration: build/SYSTEM/kyber_ref-k3-n1/
VARIANT := k$(KYBER_K)-n$(NTESTS)

# One iteration runs keypair/enc/dec three times (~30M cycles for Kyber1024)
MAX_CYCLES := $(shell echo $$(( 40000000 * $(NTESTS) )))
