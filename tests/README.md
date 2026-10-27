# Ibex custom tests (Simple System and Demo System, Verilator)

Each folder here is one test. The Makefile compiles it and runs it on one of
two Verilator models of an Ibex system, then reports PASS or FAIL:

| `SYSTEM=` | System | Source |
|---|---|---|
| `simple` (default) | Ibex Simple System: RAM, timer, simulator output/halt | `../ibex/examples/simple_system` |
| `demo` | Ibex Demo System: adds UART, GPIO, PWM, SPI, debug module | `../ibex-demo-system` |

Requirements: `riscv32-unknown-elf-gcc` on PATH (or `TOOLCHAIN_DIR=<dir>`),
`fusesoc`, `verilator`, the libelf and lz4 development headers, and Python 3
with `venv`. The first demo build creates `build/venv_demo` with the demo
system's pinned tools (lowRISC's FuseSoC fork). Your own FuseSoC is not changed.

## Quick start

```sh
cd tests
make test NAME=hello                 # simple system; first run builds the simulator (minutes)
make test NAME=hello SYSTEM=demo     # same test on the demo system
make new NAME=my_test                # creates tests/my_test/my_test.c from a template
make test NAME=my_test
make regress                         # every test on every system it supports
make regress SYSTEM=demo             # only on one system
```

## Targets

| Target | What it does |
|---|---|
| `make test NAME=X` | build sim if needed, compile X, run, PASS/FAIL (exit code 0/1) |
| `make build NAME=X` | compile only → `build/<SYSTEM>/X/X.elf`, `X.dis` |
| `make run NAME=X` | run an already compiled test |
| `make wave NAME=X` | run with waveform dump → `build/<SYSTEM>/X/sim.fst` (GTKWave) |
| `make dis NAME=X` | disassembly → `build/<SYSTEM>/X/X.dis` |
| `make new NAME=X` | create a test from a template |
| `make sim` | build only the simulator for `SYSTEM` (and `CONFIG`) |
| `make list` | list tests and the systems they run on |
| `make regress` | run all tests → `build/regress.txt` |
| `make clean [NAME=X]` | remove test outputs (keeps simulators) |
| `make clean-sim` | remove the simulator for `SYSTEM` (do this after RTL changes) |

All targets take `SYSTEM=simple|demo`.

## Variables

Set them on the command line (`make test NAME=X OPT=-O2`), or for one test in
`tests/X/test.mk`:

| Variable | Default | Meaning |
|---|---|---|
| `SYSTEM` | `simple` | system to run on |
| `SYSTEMS` | `simple demo` | systems the test supports (set in `test.mk`, e.g. `SYSTEMS := demo`) |
| `CONFIG` | `small` | simple system only: Ibex config from `ibex/ibex_configs.yaml`; each config gets its own sim |
| `ARCH` | `rv32imc_zicsr_zifencei` | `-march`, must match what the system's Ibex supports |
| `OPT` | `-Os` | optimization level |
| `MAX_CYCLES` | `2000000` | timeout, then the test fails |
| `CFLAGS_EXTRA` | | extra compiler flags, e.g. `-DN=10` |
| `EXPECT` | | golden output file in the test folder (see below) |
| `TOOLCHAIN_DIR` | | toolchain install dir if not on PATH |

## Writing a test

- Put `*.c` / `*.S` files in `tests/X/`. All of them are compiled together with
  `lib/` and the runtime of the selected system:
  - simple: `ibex/examples/sw/simple_system/common`
  - demo: `ibex-demo-system/sw/c/common` (drivers: `uart.h`, `gpio.h`, `timer.h`, `pwm.h`, `spi.h`)
- Include `test_utils.h`. It includes the right system header and gives
  `putchar`, `puts`, `puthex`, `sim_halt` on both systems. Don't also include
  `simple_system_common.h`: it breaks when included twice.
- No libc: no `printf`, and no `memcpy`. A local array with an initializer
  needs `memcpy`, so make it `static`.
- Report the result with `test_pass()` / `test_fail()`, or with
  `CHECK(cond)` / `CHECK_EQ(a, b)`, which call `test_fail()` on error.
  `read_cycles()` reads `mcycle`.
- A test that uses demo system peripherals sets `SYSTEMS := demo` in its `test.mk`.

On the demo system, tests are built with `SIM_CTRL_OUTPUT`, so `puts` and
friends write to the simulator log like on the simple system. Writing to the
UART directly (`uart_out`) ends up in `uart0.log` and is shown after the program
output. The UART is slow in simulation (about 4340 cycles per character), so wait
before halting. See `demo_periph`.

Verdict rules, checked against the program output (`ibex_simple_system.log` or
`ibex_demo_system.log` in `build/<SYSTEM>/X/`):

1. simulator error or timeout → FAIL
2. if `EXPECT` is set → output must match `tests/X/<EXPECT>` exactly (diff in `expect.diff`)
3. otherwise → `TEST FAILED` means FAIL, `TEST PASSED` means PASS, neither means FAIL

## Outputs (`build/<SYSTEM>/X/`)

| File | Content |
|---|---|
| `X.elf`, `X.dis` | program and disassembly |
| `ibex_simple_system.log` / `ibex_demo_system.log` | program output |
| `uart0.log` | demo only: UART output |
| `sim_stdout.log` | simulator stdout (cycles, performance counters) |
| `trace_core_00000000.log` | instruction trace |
| `*_pcount.csv` | performance counters |
| `result.txt` | one-line verdict |

## Example tests

| Test | Systems | Shows |
|---|---|---|
| `hello` | both | printing and passing |
| `mul_div` | both | `CHECK_EQ` on M extension results, cycle measurement |
| `golden_asm` | both | C plus assembly, `test.mk`, golden `expected.log` |
| `demo_periph` | demo | GPIO readback, timer interrupts, UART output |
