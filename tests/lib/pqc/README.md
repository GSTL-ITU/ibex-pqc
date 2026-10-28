# Post-quantum crypto support files

Shared by the PQC tests (`kyber_ref`, `kyber_cycles`, `dilithium_ref`,
`dilithium_cycles`). Not compiled into
other tests: only `lib/*.c` is, not this subfolder.

| File | Purpose |
|---|---|
| `kyber.mk` | Included by the Kyber tests' `test.mk`. Compiles `kyber/ref/*.c` from the submodule, except `randombytes.c`, and sets `KYBER_K`, `-O2` and a 32 KiB stack. |
| `dilithium.mk` | The same for Dilithium: compiles `dilithium/ref/*.c` except `randombytes.c`, sets `DILITHIUM_MODE`, `-O2`, the stack size per mode and system, and limits Dilithium5 to the simple system. |
| `randombytes.c` | Replaces `kyber/ref/randombytes.c` and `dilithium/ref/randombytes.c`, which need an OS (`getrandom`, `/dev/urandom`). A deterministic xorshift32 PRNG with a fixed seed (`RANDOMBYTES_SEED`), so runs and cycle counts are reproducible. **Not cryptographically secure.** Copied from `pulpissimo-pqc/tests/kyber_ref/randombytes.c`, with one addition: `randombytes_seed()` restarts it from another seed. It uses the `randombytes.h` of the algorithm it is built with. |
| `bench.h` | For the `*_cycles` tests: `bench_seed(i)`, the seed of run `i` (run 0: `RANDOMBYTES_SEED`, the others derived from it), and `bench_summary()`, which prints median, min and max. |
| `cycles_table.py` | Markdown table of the `*_cycles` results in `build/` for one `N_TEST`: 1 seed, median, min – max, per system. |

## Kyber

Source: the `kyber` submodule, https://github.com/pq-crystals/kyber, commit
`3edd5af5991927164edd4aacebfcbee00b8064e7` (the same as in pulpissimo-pqc).
Compiled unchanged from `ref/`:
`kem.c indcpa.c polyvec.c poly.c ntt.c cbd.c reduce.c verify.c fips202.c
symmetric-shake.c` and their headers.

Changed copies:

| File | Change |
|---|---|
| `tests/kyber_ref/test_kyber.c` | From `ref/test/`. Include paths `../kem.h` and `../randombytes.h` changed to `kem.h` and `randombytes.h`. `#define NTESTS 1000` wrapped in `#ifndef NTESTS` so the iteration count can be set from `test.mk`. Identical to the pulpissimo-pqc copy. |
| `lib/pqc/randombytes.c` | See above |

`tests/kyber_cycles/kyber_cycles.c` is based on the pulpissimo-pqc test,
extended with `N_TEST` runs with different seeds. With `N_TEST=1` it measures the same as the original.

## Dilithium

Source: the `dilithium` submodule, https://github.com/pq-crystals/dilithium,
commit `d35ba3fe5449bee3e6d43e1f296c3ca818bd36be` (the same as in
pulpissimo-pqc). Compiled unchanged from `ref/`:
`sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c fips202.c
symmetric-shake.c` and their headers.

Changed copies:

| File | Change |
|---|---|
| `tests/dilithium_ref/test_dilithium.c` | From `ref/test/`. Include paths `../randombytes.h` and `../sign.h` changed to `randombytes.h` and `sign.h`. `#define NTESTS 10000` wrapped in `#ifndef NTESTS`. Identical to the pulpissimo-pqc copy. |
| `lib/pqc/randombytes.c` | See above |

`tests/dilithium_cycles/dilithium_cycles.c` is based on the pulpissimo-pqc
test, extended with `N_TEST` runs with different seeds (each run also draws
a new message). With `N_TEST=1` it measures the same as the original. `test_dilithium.c` and
`dilithium_cycles.c` use `snprintf` and `fprintf(stderr, ...)`, which
`lib/libc_mini.c` provides.
