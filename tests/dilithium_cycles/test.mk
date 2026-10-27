include $(TESTS_DIR)/lib/pqc/dilithium.mk

# Separate build directory per parameter set: build/SYSTEM/dilithium_cycles-mode2/
VARIANT := mode$(DILITHIUM_MODE)

# Dilithium5 takes ~60M cycles in total; signing varies (rejection sampling)
MAX_CYCLES := 200000000
