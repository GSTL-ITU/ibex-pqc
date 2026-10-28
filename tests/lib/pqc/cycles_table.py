#!/usr/bin/env python3
"""Markdown table of the kyber_cycles / dilithium_cycles results.

  python3 lib/pqc/cycles_table.py [N_TEST] [-o out.md]

Reads the program output of every build/<system>/{kyber,dilithium}_cycles-*
directory for that N_TEST (default 10), i.e. after runs like
  make test NAME=kyber_cycles KYBER_K=3 N_TEST=10 SYSTEM=demo
and prints, per system: run 0 (the default seed), median, and min – max.
Missing configurations are left out.
"""

import os
import re
import sys

TESTS_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SYSTEMS = ('simple', 'demo')
LOGS = {'simple': 'ibex_simple_system.log', 'demo': 'ibex_demo_system.log'}

# test, variant prefix, parameter values, display name, operations (as printed)
ALGOS = [
    ('kyber_cycles', 'k', (2, 3, 4), lambda p: f'Kyber{256 * p}',
     [('keypair', 'crypto_kem_keypair'), ('enc', 'crypto_kem_enc'), ('dec', 'crypto_kem_dec')]),
    ('dilithium_cycles', 'mode', (2, 3, 5), lambda p: f'Dilithium{p}',
     [('keypair', 'crypto_sign_keypair'), ('sign', 'crypto_sign'), ('open', 'crypto_sign_open')]),
]

RUN = re.compile(r'^run (\d+) seed (0x[0-9a-f]+): (.*) cycles$')


def runs(path):
    """{op: [cycles of run 0, run 1, ...]} from one program output log."""
    out = {}
    for line in open(path, errors='replace'):
        m = RUN.match(line.strip())
        if not m:
            continue
        for part in m.group(3).split(', '):
            op, n = part.split()
            out.setdefault(op, []).append(int(n))
    return out


def median(v):
    s = sorted(v)
    n = len(s)
    return s[n // 2] if n % 2 else (s[n // 2 - 1] + s[n // 2]) // 2  # as bench.h


def main(argv):
    args = argv[1:]
    out_path = None
    if '-o' in args:
        i = args.index('-o')
        out_path = args[i + 1]
        del args[i:i + 2]
    n_test = int(args[0]) if args else 10
    suffix = '' if n_test == 1 else f'-n{n_test}'

    head = ['Parameter set', 'Operation']
    for s in SYSTEMS:
        head += [f'{s}: 1 seed', f'{s}: median', f'{s}: min – max']
    lines = [f'Cycle counts, {n_test} runs with different seeds (run 0 = default seed)', '',
             '| ' + ' | '.join(head) + ' |', '|---|---|' + '---:|' * (len(head) - 2)]
    found = False
    for test, prefix, params, label, ops in ALGOS:
        for p in params:
            data = {}
            for s in SYSTEMS:
                log = os.path.join(TESTS_DIR, 'build', s, f'{test}-{prefix}{p}{suffix}', LOGS[s])
                if os.path.exists(log):
                    data[s] = runs(log)
            if not any(data.values()):
                continue
            found = True
            for j, (op, fn) in enumerate(ops):
                cells = [label(p) if j == 0 else '', f'`{fn}`']
                for s in SYSTEMS:
                    v = data.get(s, {}).get(op)
                    if not v:
                        cells += ['–'] * 3
                        continue
                    if len(v) != n_test:
                        print(f'warning: {s} {test} {prefix}{p}: {len(v)} of {n_test} runs',
                              file=sys.stderr)
                    lo, hi = min(v), max(v)
                    cells += [f'{v[0]:,}', f'{median(v):,}', f'{lo:,} – {hi:,}']
                lines.append('| ' + ' | '.join(cells) + ' |')
    if not found:
        sys.exit(f'no results for N_TEST={n_test} in {TESTS_DIR}/build/')
    lines += ['', '1 seed: run 0, the default seed (RANDOMBYTES_SEED). Median of an even '
              'number of runs: mean of the two middle values.', '']
    text = '\n'.join(lines)
    if out_path:
        open(out_path, 'w').write(text)
    print(text)


if __name__ == '__main__':
    main(sys.argv)
