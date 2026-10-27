# Ibex test workspace

Runs custom C/assembly tests on the Ibex RISC-V core in Verilator, on two
systems built around it, including cycle counts of post-quantum crypto
(Kyber / ML-KEM and Dilithium / ML-DSA).

```
.
├── tests/              test suite + Makefile (see tests/README.md)
├── fpga/               FPGA area reports for the Nexys 4 DDR, core vs SoC (see fpga/README.md)
├── ibex/               submodule: lowRISC/ibex (core + Simple System)
├── ibex-demo-system/   submodule: lowRISC/ibex-demo-system (Demo System)
├── kyber/              submodule: pq-crystals/kyber (Kyber reference code, used by tests/kyber_*)
├── dilithium/          submodule: pq-crystals/dilithium (Dilithium reference code, used by tests/dilithium_*)
└── docs/               notes: running a C program by hand, original test design
```

## Getting the code

```sh
git clone --recursive <this repo>
# or, in an existing clone without the submodules:
git submodule update --init
```

`ibex`, `ibex-demo-system`, `kyber` and `dilithium` are git submodules. The parent repo stores only
the URL and the exact commit of each, so a fresh clone gets the same versions.
The demo system has its own vendored copy of Ibex (`ibex-demo-system/vendor/lowrisc_ibex`).
The `ibex` submodule is what the Simple System uses.

## Running tests

```sh
cd tests
make test NAME=hello                 # on the Simple System
make test NAME=hello SYSTEM=demo     # on the Demo System
make regress                         # everything
```

See [tests/README.md](tests/README.md) for requirements, all targets and how to write tests.

## Working with the submodules

| Task | Command |
|---|---|
| See which commit each submodule is at | `git submodule status` |
| Update a submodule to the latest upstream | `git -C ibex pull origin master` (demo: `git -C ibex-demo-system pull origin main`), then `git add ibex` and commit |
| Undo local changes inside a submodule | `git -C ibex checkout .` |
| After pulling this repo, sync the submodules | `git submodule update` |

A submodule is a full git repo checked out at a fixed commit, so you can
change, branch and commit inside it. The parent only records which commit to use.
To keep your own Ibex changes, point the submodule at your fork:
`git submodule set-url ibex <your fork URL>`.

After updating `ibex`, run `make clean-sim` in `tests/` so the simulator is rebuilt.
For the demo system, run `make clean-sim SYSTEM=demo`.
