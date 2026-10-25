"""Instruction-shape candidates, not source acceptance or a complete function map.

Ghidra bodies supply instruction ranges. Literal pointers and external branch
addresses are normalized; register operands, constants and local branch targets
remain. ADR immediates remain exact; jump-table contents need separate validation.
"""
from pathlib import Path
import json, re, csv, collections, sys
import capstone
from common import ROOT, WORK, PC_RELATIVE, ArmImage
R = ROOT
W = WORK
md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB | capstone.CS_MODE_LITTLE_ENDIAN)
md.detail = True
images = {v: ArmImage(v) for v in (316, 318, 319)}
functions = {v: {int(k): r for k, r in json.loads((W / f'A{v}-functions.json').read_text()).items()} for v in images}
maps = list(csv.DictReader((R / 'inverters/gen3-lv/a316/map/functions.tsv').open(), delimiter='\t'))
names = {int(r['addr'], 16): r['name'] for r in maps}

def fingerprint(v, start, ranges):
    image = images[v]
    instructions = [i for lo, hi in ranges for i in md.disasm(image.bytes(lo, hi - lo), lo)]
    local = {i.address: k for k, i in enumerate(instructions)}
    tokens = []
    refs = []
    for i in instructions:
        text = i.op_str
        rr = []
        p = PC_RELATIVE.search(text)
        if p and i.mnemonic.startswith(('ldr', 'vldr')):
            a = (i.address + 4 & ~3) + int(p[1], 0)
            if not image.base <= a <= image.end - 4:
                return None
            value = int.from_bytes(image.bytes(a, 4), 'little')
            category = 'flash' if image.base <= value & ~1 < image.end else 'ram' if 536870912 <= value < 536936448 else None
            label = category or hex(value)
            if category:
                rr.append((category, value))
            text = text[:p.start()] + '[literal:' + label + ']' + text[p.end():]
        elif i.group(capstone.CS_GRP_JUMP) or i.group(capstone.CS_GRP_CALL):
            match = re.search('#(0x[0-9a-f]+|\\d+)$', text)
            if match:
                target = int(match[1], 0)
                if target in local:
                    label = 'local:' + str(local[target])
                else:
                    label = 'external'
                    rr.append(('branch', target))
                text = text[:match.start()] + '#' + label
        tokens.append(i.mnemonic + ' ' + text)
        refs.append(rr)
    return (tokens, refs)
fps = {v: {a: fingerprint(v, a, r) for a, r in ff.items()} for v, ff in functions.items()}
results = {}
for old, new in ((316, 318), (316, 319), (318, 319)):
    index = collections.defaultdict(list)
    for at, f in fps[new].items():
        if f:
            index[tuple(f[0])].append(at)
    rows = []
    inherited = {r['candidates'][0]: r['name'] for r in results.get('316_318', []) if r['status'] == 'unique_shape'}
    baseline = [(int(r['addr'], 16), r['name']) for r in maps] if old == 316 else [(a, inherited.get(a, f'function_{a:08X}')) for a in sorted(functions[old])]
    for at, name in baseline:
        f = fps[old].get(at)
        hits = index.get(tuple(f[0]), []) if f else []
        row = {'name': name, 'old': at, 'status': 'body_unavailable' if f is None else 'unique_shape' if len(hits) == 1 else 'ambiguous_shape' if hits else 'no_shape_match', 'candidates': hits}
        if len(hits) == 1:
            g = fps[new][hits[0]]
            row['reference_pairs'] = [{'instruction': q, 'kind': kind, 'old': value, 'new': newvalue} for q, (a, b) in enumerate(zip(f[1], g[1])) for (kind, value), (newkind, newvalue) in zip(a, b) if kind == newkind]
        rows.append(row)
    results[f'{old}_{new}'] = rows
    print(old, new, dict(collections.Counter((r['status'] for r in rows))), flush=True)
(W / 'ARM-normalized.json').write_text(json.dumps(results, indent=2) + '\n')
