include $(TESTS_DIR)/lib/pqc/dilithium.mk

# Runs, each with its own seed (lib/pqc/bench.h): make ... N_TEST=10
N_TEST ?= 1
TEST_CFLAGS += -DN_TEST=$(N_TEST)

# Separate build directory per configuration: build/SYSTEM/dilithium_cycles-mode2/,
# or dilithium_cycles-mode2-n10/ for N_TEST=10
VARIANT := mode$(DILITHIUM_MODE)$(if $(filter-out 1,$(N_TEST)),-n$(N_TEST))

# Dilithium5 takes ~60M cycles per run; signing varies (rejection sampling)
MAX_CYCLES := $(shell echo $$(( 200000000 * $(N_TEST) )))
