# FPGA area (and timing) reports for the Nexys 4 DDR (XC7A100T), core vs SoC.
#
#   make area                      simple system, CONFIG=small
#   make area SYSTEM=demo          demo system
#   make compare                   both, plus a side-by-side summary
#   make help                      all targets and variables
#
# Uses FuseSoC to set up a Vivado project, then scripts/area.tcl for synthesis
# and reports, and scripts/area_summary.py for the core/SoC breakdown.

FPGA_DIR  := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TOP_DIR   := $(abspath $(FPGA_DIR)/..)
BUILD_DIR := $(FPGA_DIR)/build
IDLE_VMEM := $(FPGA_DIR)/data/idle.vmem

SYSTEM  ?= simple
# simple only: Ibex configuration from ibex/ibex_configs.yaml
CONFIG  ?= small
# simple only: RAM in KiB, a power of two (demo: fixed 128 KiB)
MEM_KIB ?= 128
# simple only: extra FuseSoC parameters, e.g. --RegFile=ibex_pkg::RegFileFPGA.
# Give each setting its own VARIANT (build directory suffix).
SIMPLE_OPTS ?=
VARIANT ?=
# 1: also place and route, with utilization and timing after routing
IMPL    ?= 0
JOBS    ?= $(shell nproc 2>/dev/null || echo 4)
VIVADO  ?= vivado

STAGE := $(if $(filter 1,$(IMPL)),route,opt)

ifeq ($(SYSTEM),simple)
SYS_ROOT    := $(TOP_DIR)/ibex
OUT         := $(BUILD_DIR)/simple-$(CONFIG)-$(MEM_KIB)k$(if $(VARIANT),-$(VARIANT))
PROJECT     := ibex-pqc_fpga_simple_system_nexys4ddr_0
WORK        := $(OUT)/$(PROJECT)/synth-vivado
FUSESOC_ENV :=
FUSESOC_RUN  = fusesoc --cores-root=. --cores-root=$(FPGA_DIR)/simple run --target=synth --setup \
	--build-root=$(OUT) --mapping lowrisc:prim_xilinx:all:0.1 ibex-pqc:fpga:simple_system_nexys4ddr \
	$$(util/ibex_config.py $(CONFIG) fusesoc_opts) --MemSizeKiB=$(MEM_KIB) \
	--SRAMInitFile=$(IDLE_VMEM) $(SIMPLE_OPTS)
SYS_DESC    := simple system, CONFIG=$(CONFIG), $(MEM_KIB) KiB RAM$(if $(SIMPLE_OPTS), $(SIMPLE_OPTS))
else ifeq ($(SYSTEM),demo)
# The demo system's generators need its pinned FuseSoC fork, from the venv
# that tests/ creates (tests/Makefile builds it on first use)
SYS_ROOT    := $(TOP_DIR)/ibex-demo-system
OUT         := $(BUILD_DIR)/demo
PROJECT     := lowrisc_ibex_demo_system_0
WORK        := $(OUT)/synth_nexysa7-vivado
DEMO_VENV   := $(TOP_DIR)/tests/build/venv_demo
FUSESOC_ENV := PATH=$(DEMO_VENV)/bin:$$PATH
FUSESOC_RUN  = fusesoc --cores-root=. run --target=synth_nexysa7 --setup \
	--build-root=$(OUT) lowrisc:ibex:demo_system --SRAMInitFile=$(IDLE_VMEM)
SYS_DESC    := demo system (fixed Ibex configuration, 128 KiB RAM)
else
$(error Unknown SYSTEM=$(SYSTEM), use simple or demo)
endif

.PHONY: help area compare clean check-tools

help:
	@echo "Usage:"
	@echo "  make area [SYSTEM=simple|demo]  synthesis + area report -> build/<system...>/area.md"
	@echo "  make compare                    both systems + side-by-side summary -> build/compare.md"
	@echo "  make clean                      remove build/"
	@echo ""
	@echo "Variables: SYSTEM=$(SYSTEM) CONFIG=$(CONFIG) MEM_KIB=$(MEM_KIB) IMPL=$(IMPL) JOBS=$(JOBS)"
	@echo "           SIMPLE_OPTS=$(SIMPLE_OPTS) VARIANT=$(VARIANT) VIVADO=$(VIVADO)"

check-tools:
	@command -v $(VIVADO) >/dev/null 2>&1 || { echo "Error: $(VIVADO) not found, add Vivado's bin/ to PATH or set VIVADO=<path>"; exit 1; }
ifeq ($(SYSTEM),demo)
	@test -x $(DEMO_VENV)/bin/fusesoc || $(MAKE) -C $(TOP_DIR)/tests $(DEMO_VENV)/bin/fusesoc
endif

# FuseSoC setup every time (seconds), so RTL and parameter changes are picked up
area: check-tools
	@echo "=== $(SYS_DESC)"
	@mkdir -p $(BUILD_DIR) && touch $(BUILD_DIR)/FUSESOC_IGNORE
	rm -rf $(OUT)
	cd $(SYS_ROOT) && $(FUSESOC_ENV) $(FUSESOC_RUN) > $(OUT).fusesoc.log 2>&1 || \
	  { tail -20 $(OUT).fusesoc.log; echo "Error: FuseSoC setup failed, see $(OUT).fusesoc.log"; exit 1; }
	mv $(OUT).fusesoc.log $(OUT)/fusesoc.log
	@echo "Running Vivado (a few minutes), log: $(OUT)/vivado.log"
	$(VIVADO) -mode batch -nojournal -log $(OUT)/vivado.log -source $(FPGA_DIR)/scripts/area.tcl \
	  -tclargs $(WORK) $(PROJECT) $(OUT)/reports $(IMPL) $(JOBS) > $(OUT)/vivado_stdout.log 2>&1 || \
	  { grep -E "^ERROR" $(OUT)/vivado.log | head -20; echo "Error: Vivado failed, see $(OUT)/vivado.log"; exit 1; }
	@echo "$(SYS_DESC)" > $(OUT)/description.txt
	python3 $(FPGA_DIR)/scripts/area_summary.py "$(SYSTEM)=$(OUT)/reports/util_hier_$(STAGE).rpt" \
	  -o $(OUT)/area.md
ifeq ($(IMPL),1)
	@grep -A3 "WNS(ns)" $(OUT)/reports/timing_route.rpt | head -4
endif

# Both systems with the current settings, then one summary
compare:
	$(MAKE) area SYSTEM=simple
	$(MAKE) area SYSTEM=demo
	python3 $(FPGA_DIR)/scripts/area_summary.py \
	  "simple=$$($(MAKE) -s print-out SYSTEM=simple)/reports/util_hier_$(STAGE).rpt" \
	  "demo=$(BUILD_DIR)/demo/reports/util_hier_$(STAGE).rpt" \
	  -o $(BUILD_DIR)/compare.md

.PHONY: print-out
print-out:
	@echo $(OUT)

clean:
	rm -rf $(BUILD_DIR)
