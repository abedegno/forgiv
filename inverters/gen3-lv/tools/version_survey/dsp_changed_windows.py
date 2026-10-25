"""Compare bounded windows containing unresolved DSP functions.

Window names are baseline context, not accepted new function boundaries.
"""
from normalize_dsp import *
results = json.loads((W / 'DSP-normalized.json').read_text())
byversion = {v: {r['old']: r for r in rows} for v, rows in results.items()}
common = {a for a, r in byversion['318'].items() if r['status'] == 'unique_shape' and byversion['319'][a]['status'] == 'unique_shape'}
ordered = sorted(common)
windows = []
for left, right in zip(ordered, ordered[1:]):
    names = [f['name'] for f in maps if left < int(f['addr'], 16) < right]
    if not any((byversion[v][int(f['addr'], 16)]['status'] == 'no_shape_match' for f in maps if left < int(f['addr'], 16) < right for v in ('318', '319'))):
        continue
    spans = {}
    ts = {}
    rs = {}
    for v in (318, 319):
        lv = byversion[str(v)][left]
        rv = byversion[str(v)][right]
        start = norm[v][2][lv['candidates'][0]] + lv['instruction_count']
        end = norm[v][2][rv['candidates'][0]]
        ts[v] = norm[v][0][start:end]
        rs[v] = norm[v][1][start:end]
        spans[str(v)] = [images[v][start][0], rv['candidates'][0]]
    windows.append({'baseline_functions': names, 'spans': spans, 'same_normalized_instructions': ts[318] == ts[319], 'same_address_operands': rs[318] == rs[319]})
(W / 'DSP-changed-windows.json').write_text(json.dumps(windows, indent=2) + '\n')
print([(w['baseline_functions'], w['same_normalized_instructions']) for w in windows])
