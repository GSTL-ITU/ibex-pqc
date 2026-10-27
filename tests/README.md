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
make test NAME=kyber_cycles          # Kyber768 cycle counts, ~12 s
make test NAME=dilithium_cycles      # Dilithium2 cycle counts, ~1 min
make new NAME=my_test                # creates tests/my_test/my_test.c from a template
make test NAME=my_test
make regress                         # every test on every system it supports (~6 min)
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
`tests/X/test.mk`. In a `test.mk`, use `:=`, not `?=`: the Makefile already
gives every variable a default. The command line always wins over `test.mk`.

| Variable | Default | Meaning |
|---|---|---|
| `SYSTEM` | `simple` | system to run on |
| `SYSTEMS` | `simple demo` | systems the test supports (set in `test.mk`, e.g. `SYSTEMS := demo`) |
| `CONFIG` | `small` | simple system only: Ibex config from `ibex/ibex_configs.yaml`; each config gets its own sim |
| `ARCH` | `rv32imc_zicsr_zifencei` | `-march`, must match what the system's Ibex supports |
| `OPT` | `-Os` | optimization level |
| `MAX_CYCLES` | `2000000` | timeout, then the test fails |
| `STACK_SIZE` | system's | stack in bytes, e.g. `0x8000` (see [Memory](#memory)) |
| `TRACE` | `1` | `0` skips the instruction trace: ~3x faster, no GB-sized trace file, same cycle counts. The PQC tests default to `0`. |
| `VARIANT` | | build directory suffix, `build/<SYSTEM>/X-<VARIANT>/` |
| `CFLAGS_EXTRA` | | extra compiler flags, e.g. `-DN=10` |
| `EXPECT` | | golden output file in the test folder (see below) |
| `TOOLCHAIN_DIR` | | toolchain install dir if not on PATH |

For `test.mk` only, added to with `+=`:

| Variable | Meaning |
|---|---|
| `EXTRA_SRCS` | more source files (absolute paths: use `$(TESTS_DIR)` / `$(TOP_DIR)`) |
| `EXTRA_INCS` | more include directories |
| `TEST_CFLAGS` | the test's own compiler flags (`CFLAGS_EXTRA` stays free for the command line) |

**Configuration options of a test** (like `KYBER_K`): give each one a place in
`VARIANT`, e.g. `VARIANT := k$(KYBER_K)`. Each value then builds into its own
directory, so switching doesn't rebuild everything and several settings can
run in parallel shells. Changed flags also trigger a rebuild on their own.

## Writing a test

- Put `*.c` / `*.S` files in `tests/X/`. All of them are compiled together with
  `lib/` and the runtime of the selected system:
  - simple: `ibex/examples/sw/simple_system/common`
  - demo: `ibex-demo-system/sw/c/common` (drivers: `uart.h`, `gpio.h`, `timer.h`, `pwm.h`, `spi.h`)
  A file in the test folder replaces a `lib/` or runtime file of the same name.
- **The return value of `main()` is the verdict:** 0 = PASS, anything else =
  FAIL (the value is printed). A stack overflow also fails the test (see [Memory](#memory)). `test_pass()` / `test_fail()` end the test
  immediately from anywhere; `CHECK(cond)` / `CHECK_EQ(a, b)` call `test_fail()` on error.
- Include `test_utils.h` for the system output functions (`putchar`, `puts`,
  `puthex`, `sim_halt`) and the checks. Don't include `simple_system_common.h`
  directly: it breaks when included twice.
- `lib/libc_mini.c` provides `printf`, `fprintf`, `snprintf` and their `v`
  forms (`%d %i %u %x %X %c %s %p`, width, `0`/`-` flags, `l`/`ll`/`z`),
  `memcpy`, `memmove`, `memset`, `memcmp` and `strlen`. Include `<stdio.h>` /
  `<string.h>` as usual. `stdout` and `stderr` both go to the program output.
  Nothing else from libc is available (no `malloc`, no floating-point `printf`).
- A test that uses demo system peripherals sets `SYSTEMS := demo` in its `test.mk`.

On the demo system, tests are built with `SIM_CTRL_OUTPUT`, so `puts` and
friends write to the simulator log like on the simple system. Writing to the
UART directly (`uart_out`) ends up in `uart0.log` and is shown after the program
output. The UART is slow in simulation (about 4340 cycles per character), so wait
before halting. See `demo_periph`.

Verdict rules, checked against the program output (`ibex_simple_system.log` or
`ibex_demo_system.log` in the build directory):

1. simulator error or timeout → FAIL
2. if `EXPECT` is set → output must match `tests/X/<EXPECT>` exactly, including
   the final `TEST PASSED` line (diff in `expect.diff`)
3. otherwise → `TEST FAILED` means FAIL, `TEST PASSED` means PASS, neither means FAIL
   (e.g. a crash: the trap handler prints `MEPC`/`MCAUSE` and halts)

## Memory

Both systems load the program into one RAM at `0x100000`. The stack grows down
from the top of its own region:

| System | RAM | Default stack | With `STACK_SIZE=N` |
|---|---|---|---|
| simple | 1 MiB | 32 KiB (code/data: 192 KiB) | top N bytes of the 1 MiB, the rest for code/data |
| demo | 128 KiB | **4 KiB** (code/data: 124 KiB) | top N bytes of the 128 KiB, the rest for code/data |

With `STACK_SIZE`, the Makefile writes `build/.../link.ld`: a copy of the
system's linker script with only the `ram` and `stack` lines changed. Code
with large local arrays (e.g. Kyber) needs more than the demo system's 4 KiB.

Overflowing the stack corrupts the data below it. To catch it, the `main()`
wrapper (`lib/test_main.c`) paints the stack before `main()` and fails the test
if the lowest word was overwritten. Painting takes ~2 cycles per byte of stack
(e.g. ~74k cycles for 32 KiB), outside `main()`. With `-DTEST_REPORT_STACK` in
`TEST_CFLAGS` (the PQC tests set it), it also prints `peak stack usage (whole test)`.
To measure a specific part of a test, use `stack.h`.

## Measuring cycles

`lib/cycles.h` reads the 64-bit `mcycle` counter. It has the same API as
`pulpissimo-pqc/tests/env/include/cycles.h`, so tests port unchanged:

```c
#include "cycles.h"

cycles_enable();                       // once
uint64_t overhead = cycles_overhead(); // cost of the reads themselves (4 on Ibex)
uint64_t t0 = cycles_read();
my_function();
uint64_t n = cycles_read() - t0 - overhead;
cycles_print("my_function", n);        // print only after measuring
```

Counts are core clock cycles. RAM on both systems answers in one cycle, so
the counts are what the same Ibex would take with single-cycle memory on
hardware. Don't print inside a measured region: output is a single store
here, but on hardware it would be a slow UART.

`lib/stack.h` measures peak stack usage (same API as in pulpissimo-pqc):
`stack_paint()` before the code, then `stack_max_used()`, `stack_size()` and
`stack_overflowed()`. `kyber_cycles` uses both.

## Kyber (ML-KEM)

Two tests run the CRYSTALS-Kyber reference implementation, the `kyber`
submodule ([pq-crystals/kyber](https://github.com/pq-crystals/kyber), folder `ref/`):

| Test | Purpose |
|---|---|
| `kyber_ref` | Upstream functional test (`test_kyber.c`): shared keys match, and a wrong secret key or modified ciphertext gives a different key |
| `kyber_cycles` | Cycle counts of `crypto_kem_keypair`, `crypto_kem_enc` and `crypto_kem_dec`, measured separately, plus peak stack usage. Fails if the keys differ or the stack overflowed. |

The Kyber sources are compiled straight from the submodule
(`lib/pqc/kyber.mk`, included by both `test.mk`). Only two files differ, see
[lib/pqc/README.md](lib/pqc/README.md). The main one is that `randombytes()`
is a **fixed-seed pseudorandom generator** (copied from pulpissimo-pqc), so every run
produces the same keys and the same cycle counts. It is not secure; use it
for testing only.

```sh
make test NAME=kyber_cycles                  # Kyber768, ~12 s
make test NAME=kyber_cycles KYBER_K=2        # Kyber512
make test NAME=kyber_cycles KYBER_K=4 SYSTEM=demo
make test NAME=kyber_ref                     # functional test, ~35 s
make test NAME=kyber_ref NTESTS=10           # more iterations (upstream: 1000)
```

| Variable | Default | Tests | Meaning |
|---|---|---|---|
| `KYBER_K` | `3` | both | `2` = Kyber512, `3` = Kyber768, `4` = Kyber1024 |
| `NTESTS` | `1` | `kyber_ref` | iterations of the three checks |

Both build with `-O2` (like the pulpissimo-pqc results), `STACK_SIZE=0x8000`
and `TRACE=0`, and run on both systems. Builds go to `build/<SYSTEM>/kyber_cycles-k<K>/` and
`kyber_ref-k<K>-n<NTESTS>/`.

### Results (`-O2`, `CONFIG=small`)

The two systems give identical counts: same Ibex configuration (fast
multiplier, no instruction cache), single-cycle RAM.

| Parameter set | Operation | Ibex (simple / demo) | pulpissimo-pqc Ibex | Peak stack |
|---|---|---:|---:|---:|
| Kyber512 | `crypto_kem_keypair` | 1,260,259 | 1,276,781 | 9,940 B |
| | `crypto_kem_enc` | 1,437,888 | 1,451,933 | |
| | `crypto_kem_dec` | 1,710,944 | 1,707,637 | |
| Kyber768 | `crypto_kem_keypair` | 2,082,310 | 2,102,196 | 14,836 B |
| | `crypto_kem_enc` | 2,365,661 | 2,388,046 | |
| | `crypto_kem_dec` | 2,729,324 | 2,724,962 | |
| Kyber1024 | `crypto_kem_keypair` | 3,282,301 | 3,306,081 | 20,948 B |
| | `crypto_kem_enc` | 3,610,262 | 3,649,998 | |
| | `crypto_kem_dec` | 4,069,419 | 4,074,590 | |

These are within ±1.3% of Ibex in PULPissimo (mostly fewer). The systems
differ in memory latency, Ibex configuration, and runtime and compiler
flags; this hasn't been broken down further. Peak stack is the whole test,
32 bytes more than pulpissimo-pqc reports because the `main()` wrapper's frame
is included.

**Layout drift.** The counts barely depend on the seed, but they move when
code or data moves in memory, even code outside Kyber. Adding the stack check
and `snprintf` to `lib/` changed them by up to 0.8% (e.g. Kyber512 keypair
1,269,887 → 1,260,259). Compare configurations built from the same code.

To time internal functions (e.g. `poly_ntt`, `indcpa_keypair_derand`), add
measurements to `kyber_cycles/kyber_cycles.c` the same way and include the
function's header (`poly.h`, `indcpa.h`, ...). The headers map the plain
names to Kyber's namespaced symbols.

## Dilithium (ML-DSA)

Two tests run the CRYSTALS-Dilithium reference implementation, the
`dilithium` submodule ([pq-crystals/dilithium](https://github.com/pq-crystals/dilithium),
folder `ref/`), set up the same way as Kyber:

| Test | Purpose |
|---|---|
| `dilithium_ref` | Upstream functional test (`test_dilithium.c`): sign and verify a message, check lengths and message, and check that a modified signed message fails verification |
| `dilithium_cycles` | Cycle counts of `crypto_sign_keypair`, `crypto_sign` and `crypto_sign_open`, measured separately, plus peak stack usage. Fails if verification fails or the stack overflowed. |

The sources are compiled from the submodule (`lib/pqc/dilithium.mk`).
`randombytes()` is the same fixed-seed generator as for Kyber (not secure;
testing only). The message (59 bytes) and context (14 bytes) match the
upstream test. Changes are listed in [lib/pqc/README.md](lib/pqc/README.md).

```sh
make test NAME=dilithium_cycles                      # Dilithium2, ~1 min
make test NAME=dilithium_cycles DILITHIUM_MODE=5     # ~1.5 min
make test NAME=dilithium_cycles SYSTEM=demo          # Dilithium2/3 only, see below
make test NAME=dilithium_ref                         # functional test, ~1 min
```

| Variable | Default | Tests | Meaning |
|---|---|---|---|
| `DILITHIUM_MODE` | `2` | both | `2` = Dilithium2, `3` = Dilithium3, `5` = Dilithium5 |
| `NTESTS` | `1` | `dilithium_ref` | iterations (upstream: 10000) |

Both build with `-O2` and `TRACE=0`. Builds go to
`build/<SYSTEM>/dilithium_cycles-mode<M>/` and `dilithium_ref-mode<M>-n<NTESTS>/`.

**Stack and the demo system.** The reference code keeps the public matrix and
other polynomial vectors on the stack:

| Mode | Peak stack `dilithium_cycles` | Peak stack `dilithium_ref` | `STACK_SIZE` simple | `STACK_SIZE` demo |
|---|---:|---:|---:|---:|
| 2 | 52,740 B | 64,020 B | 128 KiB | 72 KiB |
| 3 | 80,388 B | 96,452 B | 192 KiB | 84 KiB / 96 KiB (`ref`) |
| 5 | 123,380 B | 144,900 B | 192 KiB | doesn't fit |

The simple system uses the pulpissimo-pqc sizes. The demo system has 128 KiB
of RAM for code, data and stack together, so its sizes are just above the
measured peaks. **Dilithium5 runs on the simple system only.** If you change
the code and a demo run fails with `stack overflow`, raise `STACK_SIZE` in
`lib/pqc/dilithium.mk` (or `dilithium_ref/test.mk`) as far as the code still fits.

### Results (`-O2`, `CONFIG=small`)

Identical on both systems (Dilithium5: simple only):

| Parameter set | Operation | Ibex (simple / demo) | pulpissimo-pqc Ibex |
|---|---|---:|---:|
| Dilithium2 | `crypto_sign_keypair` | 4,369,190 | 4,470,567 |
| | `crypto_sign` | 27,245,969 | 27,902,534 |
| | `crypto_sign_open` | 4,658,163 | 4,769,927 |
| Dilithium3 | `crypto_sign_keypair` | 7,778,526 | 7,885,341 |
| | `crypto_sign` | 29,223,014 | 29,310,726 |
| | `crypto_sign_open` | 7,848,325 | 7,951,948 |
| Dilithium5 | `crypto_sign_keypair` | 13,153,791 | 13,342,864 |
| | `crypto_sign` | 31,453,243 | 31,619,827 |
| | `crypto_sign_open` | 13,372,558 | 13,554,960 |

0.3–2.3% fewer cycles than Ibex in PULPissimo.

**Signing time varies with the input.** Signing repeats an inner loop until a
candidate signature passes its checks (rejection sampling). The number of
repetitions depends on the key, message and randomness, so each signing count
is one sample for the fixed seed. A different seed or message can change it
by a large factor; key generation and verification vary only slightly. This
is why signing barely grows from Dilithium2 to Dilithium5. Don't compare
signing cost between modes from these numbers.

## Outputs (`build/<SYSTEM>/X/` or `X-<VARIANT>/`)

| File | Content |
|---|---|
| `X.elf`, `X.dis` | program and disassembly |
| `ibex_simple_system.log` / `ibex_demo_system.log` | program output |
| `uart0.log` | demo only: UART output |
| `sim_stdout.log` | simulator stdout (cycles, performance counters) |
| `trace_core_00000000.log` | instruction trace, not written with `TRACE=0`. Large for long tests: ~70 bytes per simulated cycle, e.g. ~0.5 GB for `kyber_cycles` (Kyber768). |
| `*_pcount.csv` | performance counters |
| `link.ld` | generated linker script, if `STACK_SIZE` is set |
| `result.txt` | one-line verdict |

## Tests

| Test | Systems | Shows |
|---|---|---|
| `hello` | both | printing and passing |
| `mul_div` | both | `CHECK_EQ` on M extension results, cycle measurement |
| `golden_asm` | both | C plus assembly, `test.mk`, golden `expected.log` |
| `demo_periph` | demo | GPIO readback, timer interrupts, UART output |
| `kyber_ref` | both | Kyber functional test |
| `kyber_cycles` | both | Kyber cycle counts and peak stack |
| `dilithium_ref` | both (mode 5: simple) | Dilithium functional test |
| `dilithium_cycles` | both (mode 5: simple) | Dilithium cycle counts and peak stack |
