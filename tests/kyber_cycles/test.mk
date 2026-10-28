include $(TESTS_DIR)/lib/pqc/kyber.mk

# Runs, each with its own seed (lib/pqc/bench.h): make ... N_TEST=10
N_TEST ?= 1
TEST_CFLAGS += -DN_TEST=$(N_TEST)

# Separate build directory per configuration: build/SYSTEM/kyber_cycles-k3/,
# or kyber_cycles-k3-n10/ for N_TEST=10
VARIANT := k$(KYBER_K)$(if $(filter-out 1,$(N_TEST)),-n$(N_TEST))

# Kyber1024 takes ~10M cycles per run
MAX_CYCLES := $(shell echo $$(( 50000000 * $(N_TEST) )))
