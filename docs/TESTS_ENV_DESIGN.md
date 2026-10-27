> **Note:** this is the original design. The implementation in `tests/` follows it,
> and has since gained demo system support (`SYSTEM=demo`). For current usage, see `tests/README.md`.

# Custom test environment for Ibex Simple System (design notes)

Goal: drop a folder under `tests/`, run `make test name=<TESTNAME>`, and have it
compiled and simulated on Ibex Simple System with Verilator, with a PASS/FAIL
result. Existing repo files are not modified. Everything lives under `tests/`.

## 1. Directory layout

```
tests/
├── Makefile              # the only entry point
├── README.md             # short usage notes (copy of section 3 below)
├── lib/                  # optional, our own helpers shared by all tests
│   └── test_utils.h      #   e.g. TEST_PASS(), TEST_FAIL(), CHECK(cond)
├── hello/                # one folder = one test
│   └── hello.c           #   <name>.c is required (or main.c, pick one convention)
├── add_loop/
│   ├── add_loop.c
│   ├── helper.S          #   any extra .c / .S in the folder is built too
│   └── test.mk           #   optional per-test overrides (see 4)
└── build/                # generated, git-ignored
    ├── sim/              #   Verilator simulator built by fusesoc
    └── hello/            #   per-test outputs: .o, .elf, .dis, logs, waves
```

## 2. What we reuse from the repo (read-only)

| Item | Path | Used for |
|---|---|---|
| Startup code | `examples/sw/simple_system/common/crt0.S` | reset vector, stack setup, bss clear, calls `main`, halts on return |
| Linker script | `examples/sw/simple_system/common/link.ld` | places code at 0x100000 |
| Runtime helpers | `examples/sw/simple_system/common/simple_system_common.{c,h}` | `puts`, `puthex`, `sim_halt`, timer |
| Register defs | `examples/sw/simple_system/common/simple_system_regs.h` | MMIO addresses |
| Simulator | core `lowrisc:ibex:ibex_simple_system` (fusesoc) | Verilator model |
| Config tool | `util/ibex_config.py` | turns a config name into fusesoc parameters |

We do **not** include `common/common.mk` directly. It builds the object files
next to the sources, so the `.o` files would end up inside `examples/sw/.../common/`
and in each test folder. Our Makefile has its own compile rules (same flags)
and puts every output in `tests/build/<name>/`, so the source tree stays clean.

## 3. User interface (make targets)

```
make test name=hello          # build sim if needed, compile hello, run, print PASS/FAIL
make build name=hello         # compile only -> tests/build/hello/hello.elf
make run name=hello           # run an already-built elf
make wave name=hello          # run with -t, writes tests/build/hello/sim.fst
make dis name=hello           # disassembly -> tests/build/hello/hello.dis
make sim                      # build only the Verilator simulator
make list                     # list all tests (every subfolder except lib/ build/)
make regress                  # run all tests, print a summary table, non-zero exit on any FAIL
make clean name=hello         # remove tests/build/hello
make clean                    # remove tests/build (keeps nothing)
```

Variables the user can override on the command line:

| Variable | Default | Meaning |
|---|---|---|
| `name` | (required for single-test targets) | test folder name |
| `CONFIG` | `small` | Ibex config from `ibex_configs.yaml` |
| `ARCH` | `rv32imc` | `-march` value; must match CONFIG (e.g. `rv32im`, `rv32imc_zba_zbb...`) |
| `CC` | `riscv32-unknown-elf-gcc` | toolchain prefix is derived from it (objdump, objcopy) |
| `MAX_CYCLES` | `1000000` | passed as `--term-after-cycles`, so a hung test stops |
| `OPT` | `-Os` | optimization level |

## 4. How a test is defined

Minimum: `tests/<name>/<name>.c` containing `main()`.

Optional `tests/<name>/test.mk`, included by the top Makefile if it exists:

```make
ARCH       := rv32im          # override per test
CFLAGS_EXTRA := -DITERATIONS=100
MAX_CYCLES := 5000000
EXPECT     := expected.log    # optional golden output file, see 6
```

Sources collected for a test:
`$(wildcard tests/<name>/*.c tests/<name>/*.S)` + common `simple_system_common.c`
+ `crt0.S` + `tests/lib/*.c` (if any).

## 5. Flow of `make test name=X`

```
           ┌────────────────────────────┐
           │ check name given & dir     │  error: "no such test: X"
           │ tests/X exists             │
           └─────────────┬──────────────┘
                         ▼
 sim stamp missing ┌────────────────────────────┐
 or CONFIG changed │ fusesoc build of simple    │  output -> tests/build/sim/
 ─────────────────▶│ system (once per CONFIG)   │  stamp file: build/sim/.config_<CONFIG>
                   └─────────────┬──────────────┘
                                 ▼
                   ┌────────────────────────────┐
                   │ compile + link (gcc)       │  -> build/X/X.elf (+ .dis)
                   └─────────────┬──────────────┘
                                 ▼
                   ┌────────────────────────────┐
                   │ cd build/X && Vibex_simple │  logs land in build/X/
                   │ _system --meminit=ram,X.elf│
                   │ --term-after-cycles=N      │
                   └─────────────┬──────────────┘
                                 ▼
                   ┌────────────────────────────┐
                   │ check result → PASS / FAIL │  exit code 0 / 1
                   └────────────────────────────┘
```

Details:

- **Simulator build**: the fusesoc command is
  `fusesoc --cores-root=<repo> run --target=sim --setup --build --build-root=tests/build/sim lowrisc:ibex:ibex_simple_system $(util/ibex_config.py $(CONFIG) fusesoc_opts)`.
  With `--build-root`, the result stays out of the repo-level `build/`. The
  binary is `tests/build/sim/sim-verilator/Vibex_simple_system`. A stamp file per
  CONFIG avoids rebuilding every time. Changing CONFIG triggers a rebuild, or
  we use one build dir per config: `tests/build/sim_<CONFIG>/`.
- **Running inside `build/X/`** matters because the simulator writes
  `ibex_simple_system.log`, `*_pcount.csv`, and `trace_core_*.log` into the
  current directory. That keeps the logs of each test apart.
- stdout of the simulator is also saved: `build/X/sim_stdout.log`.

## 6. PASS / FAIL detection

The simulator itself exits 0 whenever the program halts, so the result has
to come from the program. Proposed convention (simple, text based):

1. The test prints `TEST PASSED` or `TEST FAILED` via `puts()` before returning.
   `tests/lib/test_utils.h` gives macros for this:
   ```c
   #define CHECK(c) do { if (!(c)) { puts("CHECK failed: " #c "\n"); test_fail(); } } while (0)
   static inline void test_pass(void) { puts("TEST PASSED\n"); sim_halt(); }
   static inline void test_fail(void) { puts("TEST FAILED\n"); sim_halt(); }
   ```
2. The Makefile greps `build/X/ibex_simple_system.log`:
   - contains `TEST PASSED` and no `TEST FAILED` → **PASS**
   - contains `TEST FAILED` → **FAIL**
   - neither, e.g. timeout from `--term-after-cycles` or a crash → **FAIL (no verdict / timeout)**
3. If `EXPECT` is set in `test.mk`, the Makefile instead diffs the log against
   `tests/X/expected.log` (golden output). This is handy for "print and compare" tests.

Later we could replace the text markers with a signature write to a fixed
address, or a `main` return value. crt0 already halts on return, so the
return value would only need a small crt0 wrapper. Text is enough for now.

## 7. `make regress`

Loops over `make list`, runs `make test name=<t>` for each, tolerates failures,
collects results into `tests/build/regress.txt`:

```
hello      PASS   633 cycles
add_loop   FAIL   (timeout)
-----------------------------
1/2 passed
```

Exit code is non-zero if any test failed, so it can be used in CI later.

## 8. Things to note

- The toolchain (`riscv32-unknown-elf-gcc`) and `fusesoc` are not on PATH
  yet. They have to be installed before any of this works. The Makefile should
  check for both at the start and print a clear error.
- ARCH and CONFIG must agree. For example, a `rv32imc` binary on a config
  without compressed instructions traps. We could set ARCH automatically from
  CONFIG, as a later improvement.
- `tests/build/` should be added to `.gitignore`, ideally via `tests/.gitignore`
  so no existing file changes.
- No printf/libc: tests use `puts`/`puthex`. A tiny `printf` could be added to
  `tests/lib/` later if needed.
