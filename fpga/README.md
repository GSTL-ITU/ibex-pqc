# FPGA area: Simple System and Demo System on the Nexys 4 DDR

Synthesizes either system for the Nexys 4 DDR (XC7A100T-1CSG324, the same
board and FPGA as the Nexys A7-100T) with Vivado. Reports the resources of
the whole design, of the core alone (`ibex_top`), and of each SoC block.

```sh
cd fpga
make area                    # Simple System, CONFIG=small, ~4 min
make area SYSTEM=demo        # Demo System, ~3.5 min
make compare                 # both + side-by-side table -> build/compare.md, ~7 min
make area IMPL=1             # also place & route: post-route numbers and timing at 50 MHz
```

Requirements: Vivado on `PATH` (or `VIVADO=<path>`; tested with 2026.1, the free
edition supports the XC7A100T), `fusesoc`, and for the demo system the
FuseSoC fork in `tests/build/venv_demo`, created on first use as for the tests.

## Output

`build/<system>/area.md` is the summary, for example:

| Block | LUTs | of which LUTRAM | FFs | BRAM (36 Kb) | DSPs | share of LUTs |
|---|---:|---:|---:|---:|---:|---:|
| **Whole design** | 3,094 | 0 | 2,083 | 32 | 1 | 100% |
| **Core** (`ibex_top`) | 2,977 | 0 | 1,839 | 0 | 1 | 96% |
| &nbsp;&nbsp;ibex_core | 2,260 | 0 | 846 | 0 | 1 | 73% |
| &nbsp;&nbsp;register file | 715 | 0 | 992 | 0 | 0 | 23% |
| **SoC without core** | 117 | 0 | 244 | 32 | 0 | 4% |
| &nbsp;&nbsp;timer | 65 | 0 | 163 | 0 | 0 | 2% |
| &nbsp;&nbsp;RAM, bus, clock/reset and glue | 52 | 0 | 81 | 32 | 0 | 2% |

`build/<system>/reports/` holds Vivado's own reports: `util_opt.rpt` (whole
design), `util_hier_opt.rpt` (every hierarchy level, deeper than the summary),
and with `IMPL=1` also `util_route.rpt`, `util_hier_route.rpt` and
`timing_route.rpt`. The build directory is `build/simple-<CONFIG>-<MEM_KIB>k[-<VARIANT>]`
or `build/demo`.

## Options

| Variable | Default | Meaning |
|---|---|---|
| `SYSTEM` | `simple` | `simple` or `demo` |
| `CONFIG` | `small` | simple only: Ibex configuration from `ibex/ibex_configs.yaml` (`opentitan`, `maxperf`, ...) |
| `MEM_KIB` | `128` | simple only: RAM size, a power of two, at most 512 (the demo system has 128 KiB) |
| `SIMPLE_OPTS` | | simple only: more FuseSoC parameters, e.g. `--RegFile=ibex_pkg::RegFileFPGA` |
| `VARIANT` | | build directory suffix, give one to each `SIMPLE_OPTS` setting |
| `IMPL` | `0` | `1`: place and route too (~2x the time) |
| `JOBS` | all cores | Vivado jobs |

Synthesis is followed by `opt_design`, which removes unused logic as
implementation would. Post-route LUT counts are about 2% lower; FFs, BRAM
and DSPs are the same.

## Results (Vivado 2026.1, `IMPL=1`, post-route)

| | Simple LUTs | Simple FFs | BRAM | DSPs | Demo LUTs | Demo FFs | BRAM | DSPs |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Whole design | 3,031 | 2,083 | 32 | 1 | 5,858 | 5,121 | 32 | 10 |
| Core (`ibex_top`) | 2,915 | 1,839 | 0 | 1 | 3,289 | 1,369 | 0 | 10 |
| SoC without core | 116 | 244 | 32 | 0 | 2,569 | 3,752 | 32 | 0 |

Both meet timing at the 50 MHz system clock (worst setup slack: simple
3.2 ns, demo 0.49 ns). That is 4.8% / 9.2% of the XC7A100T's LUTs and 24%
of its block RAM (the 128 KiB RAM) each.

**The cores are configured differently**, so compare them with care:

| | Simple (`CONFIG=small`) | Demo (fixed in `ibex_demo_system.sv`) |
|---|---|---|
| Ibex version | `ibex` submodule | the demo's vendored copy (older, no CHERIoT) |
| Register file | flip-flops (`RegFileFF`): 715 LUTs, 992 FFs | LUT RAM (`RegFileFPGA`): 114 LUTs, 48 LUTRAM |
| Performance counters | none (`MHPMCounterNum=0`) | 10 (865 FFs and 9 DSPs in `cs_registers`) |
| Debug | none | debug triggers + debug module (`dm_top`) |

For a closer comparison of the cores, build the simple system with the FPGA
register file:

```sh
make area SIMPLE_OPTS=--RegFile=ibex_pkg::RegFileFPGA VARIANT=regfpga
```

That gives a core of 2,605 LUTs (48 LUTRAM) and 844 FFs (post-`opt_design`).

**Where the demo system's SoC area goes:** the UART (964 LUTs, 2,146 FFs, mostly
its two FIFOs, which are built from flip-flops) and the debug module (990 LUTs,
893 FFs). GPIO, PWM, timer and bus add ~520 LUTs. The SPI controller isn't in
the result: `top_nexysa7.sv` leaves its pins unconnected, so Vivado removes it.

## How it works

| File | Purpose |
|---|---|
| `Makefile` | FuseSoC setup of the system's synthesis target, Vivado, summary |
| `scripts/area.tcl` | Creates the FuseSoC-generated Vivado project, synthesis, `opt_design`, reports (and place & route) |
| `scripts/area_summary.py` | Turns `util_hier_*.rpt` into the core/SoC tables; several reports give a comparison |
| `simple/` | FPGA version of the Simple System (FuseSoC core `ibex-pqc:fpga:simple_system_nexys4ddr`) |
| `data/idle.vmem` | RAM contents for the area builds: one `j .` instruction (contents don't change the area) |

**Demo system:** its own `synth_nexysa7` target, unchanged. The RAM image is
replaced by `data/idle.vmem`, because the target's default path to the demo
software only resolves from inside the demo repository.

**Simple system:** the original only targets simulation and can't be
synthesized: 1 MiB RAM (more than the FPGA's ~600 KiB of block RAM),
the instruction tracer, `simulator_ctrl` (`$fwrite` / `$finish`), clock and
reset from `initial` blocks, and DPI exports. `simple/rtl/ibex_simple_system_fpga.sv`
is a copy with only those parts changed:

- `ibex_top` instead of `ibex_top_tracing`; same parameters, so `CONFIG` works as for the simulation
- RAM size `MEM_KIB` (default 128 KiB like the demo system)
- `simulator_ctrl` replaced by a register at the same address: the last
  character written goes to `LED[7:0]`, a halt request to `LED[15]`
- clock from `clkgen_xil7series` (100 MHz → 50 MHz, as on the demo system),
  reset from the CPU RESET button, pins in `simple/data/pins_nexys4ddr_simple.xdc`

Bus, RAM, timer and the core are the unchanged originals. Xilinx primitives
(e.g. the clock gate) are selected with FuseSoC's `--mapping lowrisc:prim_xilinx:all`,
like the demo system's `PRIM_DEFAULT_IMPL=prim_pkg::ImplXilinx`.

### BaseIsa: the simulation runs a CHERIoT-capable core

`ibex/examples/simple_system/ibex_simple_system.core` passes the base ISA as
a define named `BaseIsa`, but `ibex_simple_system.sv` checks `BASE_ISA`. The
setting from `CONFIG` therefore never arrives, and the Simple System
**simulation always uses the fallback `BaseIsaRV32IorCHERIoT`**: Ibex with
CHERIoT support, switched off at run time (`cheriot_enable_i` off). This is
an upstream bug. The FPGA version here reads `BaseIsa`, so `CONFIG=small`
gives the intended RV32I-only core.

The difference in area (post-`opt_design`, `CONFIG=small`):

| Core | LUTs | FFs |
|---|---:|---:|
| `BaseIsaRV32I` (default here) | 2,977 | 1,839 |
| `BaseIsaRV32IorCHERIoT` (what the simulation builds) | 3,171 | 1,916 |

To synthesize what the simulation runs:
`make area SIMPLE_OPTS=--BaseIsa=ibex_pkg::BaseIsaRV32IorCHERIoT VARIANT=cheriot`.

## Limits

- The numbers are for the FPGA. The core's share differs a lot from an ASIC,
  where e.g. the flip-flop register file is a much larger part.
- Hierarchy: Vivado's default `-flatten_hierarchy rebuilt` may move a little
  logic across module boundaries, and flattens small modules (RAM wrapper,
  bus, `sim_ctrl_fpga`), which then appear in the "glue" row.
- These builds are for area only, not for programming the board: the RAM
  holds the idle program.
