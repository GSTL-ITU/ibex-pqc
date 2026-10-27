#!/usr/bin/env python3
"""Core vs SoC area summary from Vivado hierarchical utilization reports.

  area_summary.py <name>=<util_hier.rpt> [<name>=<util_hier.rpt> ...] [-o out.md]

One report: a breakdown of that design (core, its parts, each SoC block).
Several reports: the breakdown of each, plus a side-by-side comparison.

The core is the ibex_top instance (core, register file, and whatever else
ibex_top contains for the configuration, e.g. an instruction cache). Everything
else is the SoC: RAM, bus, peripherals, debug module, clock generation.
"""

import re
import sys

# XC7A100T-1CSG324 (Nexys 4 DDR / Nexys A7-100T)
DEVICE = 'XC7A100T'
AVAILABLE = {'lut': 63400, 'lutram': 19000, 'ff': 126800, 'bram': 135, 'dsp': 240}

COLUMNS = [  # key, header
    ('lut', 'LUTs'),
    ('lutram', 'of which LUTRAM'),
    ('ff', 'FFs'),
    ('bram', 'BRAM (36 Kb)'),
    ('dsp', 'DSPs'),
]


class Node:
    def __init__(self, name, module, depth, values):
        self.name = name
        self.module = module
        self.depth = depth
        self.values = values
        self.children = []
        self.own = None  # the "(name)" row: logic in this instance itself


def parse(path):
    """Returns the root Node of the hierarchy in a report_utilization -hierarchical file."""
    header = None
    root = None
    stack = []
    for line in open(path):
        if not line.startswith('|'):
            continue
        cells = line.rstrip('\n').split('|')[1:-1]
        if header is None:
            if cells and cells[0].strip() == 'Instance':
                header = [c.strip() for c in cells]
                idx = {h: i for i, h in enumerate(header)}
            continue
        if len(cells) != len(header):
            continue
        raw = cells[0]
        depth = (len(raw) - len(raw.lstrip(' ')) - 1) // 2
        name = raw.strip()
        get = lambda h: float(cells[idx[h]].strip() or 0) if h in idx else 0.0
        values = {
            'lut': get('Total LUTs'),
            'lutram': get('LUTRAMs'),
            'ff': get('FFs'),
            'bram': get('RAMB36') + get('RAMB18') / 2,
            'dsp': get('DSP Blocks') or get('DSP48 Blocks'),
        }
        node = Node(name, cells[1].strip(), depth, values)
        while stack and stack[-1].depth >= depth:
            stack.pop()
        if name.startswith('(') and stack:
            stack[-1].own = node
            continue
        if stack:
            stack[-1].children.append(node)
        else:
            root = node
        stack.append(node)
    if root is None:
        sys.exit(f'error: no utilization table in {path}')
    return root


def find(node, pred, path=''):
    """First node (depth-first) for which pred(node) is true, with its path."""
    here = f'{path}/{node.name}' if path else node.name
    if pred(node):
        return node, here
    for c in node.children:
        r = find(c, pred, here)
        if r:
            return r
    return None


def sub(a, b):
    return {k: a[k] - b[k] for k in a}


def total(nodes):
    t = {k: 0.0 for k, _ in COLUMNS}
    for n in nodes:
        for k in t:
            t[k] += n.values[k]
    return t


def fmt(v, key):
    if key == 'bram':
        return f'{v:g}'
    return f'{int(round(v)):,}'


def row(label, values, total_values=None):
    cells = [fmt(values[k], k) for k, _ in COLUMNS]
    if total_values is not None:
        lut = total_values['lut']
        share = f'{100 * values["lut"] / lut:.0f}%' if lut else '-'
        cells.append(share)
    return '| ' + label + ' | ' + ' | '.join(cells) + ' |'


def breakdown(name, root):
    """(markdown lines, summary dict) for one design."""
    found = find(root, lambda n: n.module.startswith('ibex_top'))
    if not found:
        sys.exit(f'error: no ibex_top instance in the {name} report')
    core, core_path = found

    # The system instance is the parent of the core
    parent_path = core_path.rsplit('/', 1)[0]
    system = root
    for part in parent_path.split('/')[1:]:
        system = next(c for c in system.children if c.name == part)

    soc = sub(root.values, core.values)
    out = [f'### {name}', '']
    out.append(f'| Block | ' + ' | '.join(h for _, h in COLUMNS) + ' | share of LUTs |')
    out.append('|---|' + '---:|' * (len(COLUMNS) + 1))
    out.append(row(f'**Whole design** (`{root.name}`)', root.values, root.values))
    out.append(row(f'**Core** (`{core.name}`: {core.module})', core.values, root.values))
    for c in core.children:
        out.append(row(f'&nbsp;&nbsp;{c.name} ({c.module})', c.values, root.values))
    rest = sub(core.values, total(core.children))
    if any(abs(v) > 0.01 for v in rest.values()):
        out.append(row('&nbsp;&nbsp;other core logic', rest, root.values))
    out.append(row('**SoC without core**', soc, root.values))
    blocks = [c for c in system.children if c is not core]
    for c in blocks:
        out.append(row(f'&nbsp;&nbsp;{c.name} ({c.module})', c.values, root.values))
    # Instances above the system (e.g. clock generation), if the core is nested
    outer = []
    node = root
    for part in parent_path.split('/')[1:]:
        outer += [c for c in node.children if c.name != part]
        node = next(c for c in node.children if c.name == part)
    for c in outer:
        out.append(row(f'&nbsp;&nbsp;{c.name} ({c.module})', c.values, root.values))
    unlisted = sub(soc, total(blocks + outer))
    out.append(row('&nbsp;&nbsp;RAM, bus, clock/reset and glue (not kept as own instances)', unlisted, root.values))
    out.append('')
    out.append(f'Device usage ({DEVICE}): ' + ', '.join(
        f'{h.replace("of which ", "")} {100 * root.values[k] / AVAILABLE[k]:.1f}%'
        for k, h in COLUMNS))
    out.append('')
    return out, {'total': root.values, 'core': core.values, 'soc': soc}


def main(argv):
    args = argv[1:]
    out_path = None
    if '-o' in args:
        i = args.index('-o')
        out_path = args[i + 1]
        del args[i:i + 2]
    if not args:
        sys.exit(__doc__)

    lines = ['# FPGA area: core vs SoC', '']
    summaries = {}
    sections = []
    for a in args:
        name, path = a.split('=', 1)
        sec, summ = breakdown(name, parse(path))
        sections += sec
        summaries[name] = summ

    if len(summaries) > 1:
        lines += ['## Comparison', '']
        names = list(summaries)
        lines.append('| | ' + ' | '.join(
            f'{n} {h}' for n in names for k, h in COLUMNS if k in ('lut', 'ff', 'bram', 'dsp')) + ' |')
        lines.append('|---|' + '---:|' * (4 * len(names)))
        for part, label in (('total', 'Whole design'), ('core', 'Core (ibex_top)'),
                            ('soc', 'SoC without core')):
            cells = [fmt(summaries[n][part][k], k) for n in names
                     for k in ('lut', 'ff', 'bram', 'dsp')]
            lines.append(f'| {label} | ' + ' | '.join(cells) + ' |')
        lines.append('')

    lines += ['## Breakdown', ''] + sections
    lines += [
        'BRAM counts 36 Kb blocks (a RAMB18 is half). "share of LUTs" is the part of the',
        'whole design\'s LUTs. Small modules that Vivado flattens into their parent',
        '(RAM wrapper, bus, clock and reset, ...) show up in the last SoC row.',
        '',
    ]
    text = '\n'.join(lines)
    if out_path:
        open(out_path, 'w').write(text)
    print(text)


if __name__ == '__main__':
    main(sys.argv)
