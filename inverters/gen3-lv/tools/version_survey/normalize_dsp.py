"""C28x instruction-shape candidates from native TI disassembly.

DP tracking is linear and address classification heuristic. A match cannot
prove identical data bindings, aliasing, or source logic. All operands removed
from the signature are retained as reference pairs for subsequent checking.
"""
from pathlib import Path
import json, csv, re, collections, hashlib, struct
from common import ROOT, WORK
R = ROOT
W = WORK
B = 4096000
maps = list(csv.DictReader((R / 'inverters/gen3-lv/d316/map/functions.tsv').open(), delimiter='\t'))
images = {v: json.loads((W / f'D{v}-instructions.json').read_text()) for v in (316, 318, 319)}

def tokenize(rows):
    tokens = []
    refs = []
    page = None
    for at, size, text in rows:
        x = text
        rr = []
        z = re.fullmatch('MOVW DP, #0x([a-f0-9]+)', x)
        if z:
            page = int(z[1], 16)
            if 768 <= page < 1280 or 64000 <= page < 65024:
                x = 'MOVW DP, #PAGE'
        if page is not None and (768 <= page < 1280 or 64000 <= page < 65024):
            for z in re.finditer('@0x([a-f0-9]+)', x):
                rr.append(('ram' if page < 1280 else 'rom', page << 6 | int(z[1], 16)))
            x = re.sub('@0x[0-9a-f]+', '@DATA', x)
        z = re.match('(LCR|LB|LC|FFC) 0x([0-9a-f]+)', x)
        if z:
            rr.append(('call' if z[1] in ('LCR', 'LC', 'FFC') else 'tail', int(z[2], 16)))
            x = x[:z.start(2)] + 'CODE' + x[z.end(2):]
        z = re.match('MOVL XAR\\d, #0x([0-9a-f]+)', x)
        if z:
            value = int(z[1], 16)
            if 32768 <= value < 81920 and value not in (65535, 65536) or B <= value < 4161536:
                rr.append(('rom' if value >= B else 'ram', value))
                x = x[:z.start(1)] + 'PTR' + x[z.end(1):]
        if text.startswith('LRETR'):
            page = None
        tokens.append(x)
        refs.append(rr)
    return (tokens, refs)
norm = {}
ids = {}
for v, rows in images.items():
    ts, rs = tokenize(rows)
    norm[v] = (ts, rs, {r[0]: i for i, r in enumerate(rows)})
    for t in ts:
        ids.setdefault(t, len(ids) + 1)
streams = {v: struct.pack('<%dI' % len(ts), *[ids[t] for t in ts]) for v, (ts, _, _) in norm.items()}
results = {}
for version in (318, 319):
    rows = []
    for f in maps:
        start = int(f['addr'], 16)
        size = int(f['size'])
        end = start + size
        lookup = norm[316][2]
        if start not in lookup:
            rows.append({'name': f['name'], 'old': start, 'status': 'baseline_boundary_unavailable'})
            continue
        i = lookup[start]
        j = i
        while j < len(images[316]) and images[316][j][0] < end:
            j += 1
        if j <= i or images[316][j - 1][0] + images[316][j - 1][1] != end:
            rows.append({'name': f['name'], 'old': start, 'status': 'baseline_end_unavailable'})
            continue
        signature = streams[316][i * 4:j * 4]
        hits = []
        pos = 0
        while signature:
            k = streams[version].find(signature, pos)
            if k < 0:
                break
            if k % 4 == 0:
                hits.append(k // 4)
            if len(hits) > 50:
                break
            pos = k + 1
        result = {'name': f['name'], 'old': start, 'size_words': size, 'kind': f['kind'], 'instruction_count': j - i, 'status': 'unique_shape' if len(hits) == 1 else 'ambiguous_shape' if hits else 'no_shape_match', 'candidates': [images[version][k][0] for k in hits]}
        if len(hits) == 1:
            k = hits[0]
            result['reference_pairs'] = [{'instruction': q, 'kind': a, 'old': b, 'new': d} for q in range(j - i) for (a, b), (c, d) in zip(norm[316][1][i + q], norm[version][1][k + q]) if a == c]
        rows.append(result)
    results[str(version)] = rows
    print(version, dict(collections.Counter((r['status'] for r in rows))), flush=True)
(W / 'DSP-normalized.json').write_text(json.dumps(results, indent=2) + '\n')
