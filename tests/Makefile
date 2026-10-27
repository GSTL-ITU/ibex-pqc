# Custom test environment for Ibex, on two systems (Verilator):
#   SYSTEM=simple   ../ibex/examples/simple_system   (default)
#   SYSTEM=demo     ../ibex-demo-system
#
#   make test name=hello              build sim (if needed), compile, run, PASS/FAIL
#   make test name=hello SYSTEM=demo  same on the demo system
#   make help                         list all targets and variables
#
# Each folder under tests/ (except lib/ and build/) is one test. All sources
# (*.c, *.S) in the folder are compiled. An optional test.mk in the folder can
# override the variables below for that test, e.g. SYSTEMS := demo.

TESTS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TOP_DIR   := $(abspath $(TESTS_DIR)/..)
LIB_DIR   := $(TESTS_DIR)/lib
BUILD_DIR := $(TESTS_DIR)/build

ALL_SYSTEMS := simple demo

# ---------------------------------------------------------------- options ---
SYSTEM     ?= simple
CONFIG     ?= small
ARCH       ?= rv32imc_zicsr_zifencei
ABI        ?= ilp32
OPT        ?= -Os
MAX_CYCLES ?= 2000000
CFLAGS_EXTRA ?=
EXPECT     ?=
# Systems a test runs on; a test.mk can narrow it, e.g. SYSTEMS := demo
SYSTEMS    ?= $(ALL_SYSTEMS)
# Set to the toolchain install dir (containing bin/) if it is not on PATH
TOOLCHAIN_DIR ?=

# Per-test overrides (command line still wins)
ifneq ($(name),)
-include $(TESTS_DIR)/$(name)/test.mk
endif

# ---------------------------------------------------------------- systems ---
# SYS_ROOT     repo the system lives in (FuseSoC cores root)
# SYS_CORE     FuseSoC core of the Verilator simulation
# SYS_SW       software support dir (headers, runtime sources)
# SYS_SRCS     runtime sources compiled into every test
# SYS_LD       linker script
# SYS_DEFS     defines for tests and runtime
# SYS_LOG      file the program output (SIM_CTRL) is written to
# SYS_SIM_DIR  FuseSoC build root, SYS_SIM_WORK/SYS_SIM_BIN inside it
ifeq ($(SYSTEM),simple)
SYS_ROOT := $(TOP_DIR)/ibex
SYS_CORE := lowrisc:ibex:ibex_simple_system
SYS_SW   := $(SYS_ROOT)/examples/sw/simple_system/common
SYS_SRCS := $(SYS_SW)/simple_system_common.c $(SYS_SW)/crt0.S
SYS_LD   := $(SYS_SW)/link.ld
SYS_DEFS := -DTEST_SYSTEM_SIMPLE
SYS_LOG  := ibex_simple_system.log
SYS_FUSESOC_OPTS = $$(util/ibex_config.py $(CONFIG) fusesoc_opts)
SYS_SIM_ENV  :=
SYS_SIM_DEPS :=
SYS_SIM_DIR  := $(BUILD_DIR)/sim_simple_$(CONFIG)
SYS_SIM_WORK := $(SYS_SIM_DIR)/lowrisc_ibex_ibex_simple_system_0/sim-verilator
SYS_SIM_BIN  := $(SYS_SIM_WORK)/Vibex_simple_system
SYS_DESC := CONFIG=$(CONFIG)
else ifeq ($(SYSTEM),demo)
# The demo system has a fixed Ibex config (its own vendored Ibex), CONFIG is ignored.
# SIM_CTRL_OUTPUT sends puts/putchar to the simulator log instead of the UART.
SYS_ROOT := $(TOP_DIR)/ibex-demo-system
SYS_CORE := lowrisc:ibex:demo_system
SYS_SW   := $(SYS_ROOT)/sw/c/common
SYS_SRCS := $(addprefix $(SYS_SW)/,demo_system.c uart.c timer.c gpio.c pwm.c spi.c crt0.S)
SYS_LD   := $(SYS_ROOT)/sw/common/link.ld
SYS_DEFS := -DTEST_SYSTEM_DEMO -DSIM_CTRL_OUTPUT
SYS_LOG  := ibex_demo_system.log
SYS_FUSESOC_OPTS =
# Its vendored generators need lowRISC's FuseSoC fork, which is kept in a
# private venv so the system FuseSoC (used for the simple system) is untouched.
DEMO_VENV    := $(BUILD_DIR)/venv_demo
SYS_SIM_ENV  := PATH=$(DEMO_VENV)/bin:$$PATH
SYS_SIM_DEPS := $(DEMO_VENV)/bin/fusesoc
SYS_SIM_DIR  := $(BUILD_DIR)/sim_demo
# The FuseSoC fork puts the work dir right under the build root
SYS_SIM_WORK := $(SYS_SIM_DIR)/sim-verilator
SYS_SIM_BIN  := $(SYS_SIM_WORK)/Vtop_verilator
SYS_DESC := demo system
else
$(error Unknown SYSTEM=$(SYSTEM), use one of: $(ALL_SYSTEMS))
endif

# Extra Verilator flags. Newer Verilator versions make UNOPTFLAT fatal on Ibex.
SIM_VERILATOR_FLAGS ?= -Wno-UNOPTFLAT
SIM_JOBS ?= $(shell nproc 2>/dev/null || echo 4)

# --------------------------------------------------------------- compiler ---
# CC has a built-in default ("cc"), so ?= would not work for it
ifeq ($(origin CC),default)
CC := $(if $(TOOLCHAIN_DIR),$(TOOLCHAIN_DIR)/bin/)riscv32-unknown-elf-gcc
endif
CROSS_COMPILE := $(patsubst %-gcc,%-,$(CC))
OBJDUMP := $(CROSS_COMPILE)objdump
SIZE    := $(CROSS_COMPILE)size

CFLAGS := -march=$(ARCH) -mabi=$(ABI) -static -mcmodel=medany -Wall -g $(OPT) \
	-fvisibility=hidden -nostdlib -nostartfiles -ffreestanding \
	-I$(SYS_SW) -I$(LIB_DIR) $(SYS_DEFS) $(CFLAGS_EXTRA)
LIBS := -lgcc -Wl,--no-warn-rwx-segments

# ------------------------------------------------------------------ tests ---
TESTS := $(sort $(filter-out lib build,$(notdir $(patsubst %/,%,$(wildcard $(TESTS_DIR)/*/)))))

TEST_DIR := $(TESTS_DIR)/$(name)
OUT      := $(BUILD_DIR)/$(SYSTEM)/$(name)
REL_OUT  := build/$(SYSTEM)/$(name)
OBJ_DIR  := $(OUT)/obj
ELF      := $(OUT)/$(name).elf

SRCS := $(wildcard $(TEST_DIR)/*.c $(TEST_DIR)/*.S $(LIB_DIR)/*.c $(LIB_DIR)/*.S) $(SYS_SRCS)
OBJS := $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(notdir $(basename $(SRCS)))))

# Test folder first so a test can shadow a runtime file of the same name
vpath %.c $(TEST_DIR) $(LIB_DIR) $(SYS_SW)
vpath %.S $(TEST_DIR) $(LIB_DIR) $(SYS_SW)

.PHONY: help test build run wave dis sim list regress new clean clean-sim \
	check-name check-tools

help:
	@echo "Usage:"
	@echo "  make test name=X       build sim if needed, compile X, run, report PASS/FAIL"
	@echo "  make build name=X      compile only -> build/SYSTEM/X/X.elf"
	@echo "  make run name=X        run an already compiled test"
	@echo "  make wave name=X       run with waveform dump -> build/SYSTEM/X/sim.fst"
	@echo "  make dis name=X        disassembly -> build/SYSTEM/X/X.dis"
	@echo "  make new name=X        create tests/X from a template"
	@echo "  make sim               build the Verilator simulator for SYSTEM (and CONFIG)"
	@echo "  make list              list tests and the systems they run on"
	@echo "  make regress           run all tests on all systems (or only SYSTEM=...)"
	@echo "  make clean [name=X]    remove build/*/X (or all test outputs)"
	@echo "  make clean-sim         remove the simulator for SYSTEM (and CONFIG)"
	@echo ""
	@echo "Variables: SYSTEM=$(SYSTEM) ($(ALL_SYSTEMS)) CONFIG=$(CONFIG) (simple only)"
	@echo "           ARCH=$(ARCH) OPT=$(OPT) MAX_CYCLES=$(MAX_CYCLES)"
	@echo "           CC=$(CC) TOOLCHAIN_DIR=$(TOOLCHAIN_DIR) CFLAGS_EXTRA=$(CFLAGS_EXTRA)"

check-name:
ifeq ($(name),)
	$(error name is not set, e.g. make $(MAKECMDGOALS) name=hello)
endif
ifneq ($(filter lib build,$(name)),)
	$(error "$(name)" is not a test: tests/lib holds shared helpers, tests/build is output. Run make list)
endif
	@test -d $(TEST_DIR) || { echo "Error: no such test: $(name) (tests/$(name) not found)"; exit 1; }
ifeq ($(filter $(SYSTEM),$(SYSTEMS)),)
	$(error Test $(name) does not support SYSTEM=$(SYSTEM), only: $(SYSTEMS) (see tests/$(name)/test.mk))
endif

check-tools:
	@command -v $(CC) >/dev/null 2>&1 || { echo "Error: $(CC) not found. Add the toolchain to PATH or set TOOLCHAIN_DIR=<dir>"; exit 1; }

# Prints "<test> <systems>", systems read from each test.mk
list:
	@for t in $(TESTS); do \
	  s=$$($(MAKE) --no-print-directory -s name=$$t print-systems); \
	  printf '%-20s %s\n' $$t "$$s"; \
	done

.PHONY: print-systems
print-systems:
	@echo $(SYSTEMS)

# ------------------------------------------------------------------- sim ----
sim: $(SYS_SIM_BIN)

$(SYS_SIM_BIN): | $(SYS_SIM_DEPS)
	@test -f $(SYS_ROOT)/.git || test -d $(SYS_ROOT)/.git || { echo "Error: $(SYS_ROOT) is missing, run: git submodule update --init"; exit 1; }
	@$(SYS_SIM_ENV) command -v fusesoc >/dev/null 2>&1 || { echo "Error: fusesoc not found"; exit 1; }
	@echo '#include <libelf.h>' | c++ -E -x c++ - >/dev/null 2>&1 || { echo "Error: libelf.h not found, install elfutils-libelf-devel (Fedora) or libelf-dev (Debian/Ubuntu)"; exit 1; }
	@echo '#include <lz4.h>' | c++ -E -x c++ - >/dev/null 2>&1 || { echo "Error: lz4.h not found (needed by Verilator FST tracing), install lz4-devel (Fedora) or liblz4-dev (Debian/Ubuntu)"; exit 1; }
	@mkdir -p $(BUILD_DIR)
	@touch $(BUILD_DIR)/FUSESOC_IGNORE
	cd $(SYS_ROOT) && $(SYS_SIM_ENV) fusesoc --cores-root=. run --target=sim --tool=verilator --setup \
		--build-root=$(SYS_SIM_DIR) $(SYS_CORE) $(SYS_FUSESOC_OPTS)
	echo 'VERILATOR_OPTIONS += $(SIM_VERILATOR_FLAGS)' >> $(SYS_SIM_WORK)/config.mk
	$(MAKE) -C $(SYS_SIM_WORK) -j$(SIM_JOBS)

$(BUILD_DIR)/venv_demo/bin/fusesoc:
	python3 -m venv $(BUILD_DIR)/venv_demo
	$(BUILD_DIR)/venv_demo/bin/pip install -q -r $(TOP_DIR)/ibex-demo-system/python-requirements.txt

clean-sim:
	rm -rf $(SYS_SIM_DIR)

# ----------------------------------------------------------------- build ----
build: check-name check-tools $(ELF) $(OUT)/$(name).dis

$(ELF): $(OBJS) $(SYS_LD)
	$(CC) $(CFLAGS) -T $(SYS_LD) $(OBJS) -o $@ $(LIBS)
	@$(SIZE) $@

dis: check-name check-tools $(OUT)/$(name).dis

$(OUT)/$(name).dis: $(ELF)
	$(OBJDUMP) -fhSD $< > $@

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

$(OBJ_DIR)/%.o: %.S | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

$(OBJ_DIR):
	@mkdir -p $@

# Rebuild when flags change (e.g. a different ARCH or OPT)
$(OBJS): $(OUT)/.flags
$(OUT)/.flags: FORCE | $(OBJ_DIR)
	@echo '$(CC) $(CFLAGS)' | cmp -s - $@ || echo '$(CC) $(CFLAGS)' > $@
FORCE:

-include $(OBJS:.o=.d)

# ------------------------------------------------------------------- run ----
# Verdict, from the program output ($(SYS_LOG)):
#   EXPECT set in test.mk  -> output must match tests/X/$(EXPECT)
#   otherwise              -> "TEST PASSED" present and "TEST FAILED" absent
# On the demo system, UART output goes to uart0.log and is shown as well.
# The result line is also written to build/SYSTEM/X/result.txt for regress.
SIM_ARGS = --meminit=ram,$(name).elf --term-after-cycles=$(MAX_CYCLES)

define run_sim
	@cd $(OUT) && rm -f $(SYS_LOG) uart0.log result.txt expect.diff && \
	echo "=== Running $(name) on $(SYSTEM) ($(SYS_DESC), MAX_CYCLES=$(MAX_CYCLES))" && \
	$(SYS_SIM_BIN) $(SIM_ARGS) $(1) > sim_stdout.log 2>&1 < /dev/null; \
	rc=$$?; \
	touch $(SYS_LOG); \
	echo "--- program output ---"; cat $(SYS_LOG); echo "----------------------"; \
	if [ -s uart0.log ]; then echo "--- uart0 output ---"; tr -d '\r' < uart0.log; echo "--------------------"; fi; \
	cycles=$$(sed -n 's/^Executed cycles: *//p' sim_stdout.log); \
	if [ $$rc -ne 0 ]; then verdict="FAIL (simulator exit code $$rc, see $(REL_OUT)/sim_stdout.log)"; \
	elif grep -q "Simulation timeout" sim_stdout.log; then verdict="FAIL (timeout after $(MAX_CYCLES) cycles)"; \
	elif [ -n "$(EXPECT)" ]; then \
	  if diff -u $(TEST_DIR)/$(EXPECT) $(SYS_LOG) > expect.diff; then verdict=PASS; \
	  else verdict="FAIL (output differs from $(EXPECT), see $(REL_OUT)/expect.diff)"; fi; \
	elif grep -q "TEST FAILED" $(SYS_LOG); then verdict=FAIL; \
	elif grep -q "TEST PASSED" $(SYS_LOG); then verdict=PASS; \
	else verdict="FAIL (no TEST PASSED/TEST FAILED in output)"; fi; \
	echo "$(name) $(SYSTEM) $$verdict $${cycles:-?} cycles" > result.txt; \
	echo "=== $(name) [$(SYSTEM)]: $$verdict ($${cycles:-?} cycles)"; \
	case "$$verdict" in PASS) exit 0;; *) exit 1;; esac
endef

test: check-name check-tools $(SYS_SIM_BIN) $(ELF) $(OUT)/$(name).dis
	$(call run_sim,)

run: check-name
	@test -f $(ELF) || { echo "Error: $(ELF) not built, run make build name=$(name) SYSTEM=$(SYSTEM)"; exit 1; }
	@test -x $(SYS_SIM_BIN) || { echo "Error: simulator not built, run make sim SYSTEM=$(SYSTEM)"; exit 1; }
	$(call run_sim,)

wave: check-name check-tools $(SYS_SIM_BIN) $(ELF) $(OUT)/$(name).dis
	$(call run_sim,-t)

# --------------------------------------------------------------- regress ----
# All systems, unless SYSTEM is given on the command line. Tests whose test.mk
# excludes a system are skipped on it.
ifeq ($(origin SYSTEM),command line)
REGRESS_SYSTEMS := $(SYSTEM)
else
REGRESS_SYSTEMS := $(ALL_SYSTEMS)
endif

regress:
	@for s in $(REGRESS_SYSTEMS); do $(MAKE) --no-print-directory sim SYSTEM=$$s || exit 1; done
	@pass=0; total=0; skip=0; summary=$(BUILD_DIR)/regress.txt; : > $$summary; \
	for s in $(REGRESS_SYSTEMS); do \
	  for t in $(TESTS); do \
	    case " $$($(MAKE) --no-print-directory -s name=$$t print-systems) " in \
	      *" $$s "*) ;; \
	      *) skip=$$((skip+1)); continue;; \
	    esac; \
	    total=$$((total+1)); \
	    log=$(BUILD_DIR)/$$s/regress_$$t.log; mkdir -p $(BUILD_DIR)/$$s; \
	    rm -f $(BUILD_DIR)/$$s/$$t/result.txt; \
	    if $(MAKE) --no-print-directory test name=$$t SYSTEM=$$s > $$log 2>&1; then pass=$$((pass+1)); fi; \
	    if [ -f $(BUILD_DIR)/$$s/$$t/result.txt ]; then cat $(BUILD_DIR)/$$s/$$t/result.txt >> $$summary; \
	    else echo "$$t $$s FAIL (build error, see build/$$s/regress_$$t.log)" >> $$summary; fi; \
	  done; \
	done; \
	echo "------------------------------------------"; column -t -l 3 $$summary 2>/dev/null || cat $$summary; \
	echo "------------------------------------------"; \
	echo "$$pass/$$total passed ($$skip skipped: system not supported by test)"; \
	[ $$pass -eq $$total ]

# ------------------------------------------------------------------- new ----
new:
ifeq ($(name),)
	$(error name is not set, e.g. make new name=my_test)
endif
	@test ! -e $(TEST_DIR) || { echo "Error: tests/$(name) already exists"; exit 1; }
	@mkdir -p $(TEST_DIR)
	@printf '%s\n' \
	  '#include "test_utils.h"  // also includes the system header' \
	  '' \
	  'int main(void) {' \
	  '  puts("$(name)\n");' \
	  '' \
	  '  CHECK(1 + 1 == 2);' \
	  '' \
	  '  test_pass();' \
	  '  return 0;' \
	  '}' > $(TEST_DIR)/$(name).c
	@echo "Created tests/$(name)/$(name).c, run: make test name=$(name) [SYSTEM=demo]"

# ----------------------------------------------------------------- clean ----
clean:
ifneq ($(name),)
	rm -rf $(addsuffix /$(name),$(addprefix $(BUILD_DIR)/,$(ALL_SYSTEMS)))
else
	rm -rf $(addprefix $(BUILD_DIR)/,$(ALL_SYSTEMS)) $(BUILD_DIR)/regress.txt
endif
