include $(TESTS_DIR)/lib/pqc/kyber.mk

# Separate build directory per parameter set: build/SYSTEM/kyber_cycles-k3/
VARIANT := k$(KYBER_K)

# Kyber1024 takes ~10M cycles in total
MAX_CYCLES := 50000000
