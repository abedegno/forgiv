"""Cross-check calls against independent unique-shape entry candidates.

RAM run addresses follow the startup copy source. Unresolved calls remain
unresolved; unchanged shapes with moved RAM operands are listed explicitly.
"""
from pathlib import Path
import json, csv, collections, re
from common import ROOT, WORK
R = ROOT
W = WORK
d = json.loads((W / 'DSP-normalized.json').read_text())
maps = {r['name']: r for r in csv.DictReader((R / 'inverters/gen3-lv/d316/map/functions.tsv').open(), delimiter='\t')}
summary = {}
for v, rows in d.items():
    unique = {r['old']: r['candidates'][0] for r in rows if r['status'] == 'unique_shape'}
    loadstart = next((x['new'] for r in rows if r['name'] == 'init_system' for x in r['reference_pairs'] if x['old'] == 4135863))
    for r in rows:
        if r['status'] != 'unique_shape':
            continue
        run = re.search('run=0x([a-fA-F0-9]+)', maps[r['name']].get('notes', ''))
        if run:
            unique[int(run[1], 16)] = 32768 + r['candidates'][0] - loadstart if r['old'] < 4145152 else int(run[1], 16)
    counts = collections.Counter()
    mismatch = []
    bindings = []
    for r in rows:
        rr = r.get('reference_pairs', [])
        for ref in rr:
            if ref['kind'] in ('call', 'tail'):
                expected = unique.get(ref['old'])
                if expected is None:
                    counts['unresolved_call'] += 1
                elif expected == ref['new']:
                    counts['consistent_call'] += 1
                else:
                    counts['different_call_binding'] += 1
                    mismatch.append({'function': r['name'], **ref, 'expected': expected})
        changed = [ref for ref in rr if ref['kind'] == 'ram' and ref['old'] != ref['new']]
        if changed:
            bindings.append({'name': r['name'], 'references': changed})
    summary[v] = {'shape_counts': dict(collections.Counter((r['status'] for r in rows))), 'call_checks': dict(counts), 'call_mismatches': mismatch, 'changed_ram_bindings': bindings, 'no_shape_match': [r['name'] for r in rows if r['status'] == 'no_shape_match'], 'ramfunc_load_start': loadstart}
    print(v, summary[v]['call_checks'], 'RAM-binding functions', len(bindings), flush=True)
a = {r['name']: r for r in d['318'] if r['status'] == 'unique_shape'}
b = {r['name']: r for r in d['319'] if r['status'] == 'unique_shape'}
common = sorted(a.keys() & b.keys())
diff = []
for name in common:
    x = a[name]['reference_pairs']
    y = b[name]['reference_pairs']
    bindings = [{'instruction': p['instruction'], 'kind': p['kind'], 'from': p['new'], 'to': q['new']} for p, q in zip(x, y) if p['kind'] == 'ram' and p['new'] != q['new']]
    if bindings:
        diff.append({'name': name, 'references': bindings})
summary['318_319'] = {'common_unique_316_shapes': len(common), 'changed_ram_bindings': diff, 'lost_316_shape': sorted(a.keys() - b.keys()), 'gained_316_shape': sorted(b.keys() - a.keys()), 'limitation': 'does not cover functions whose 316 shape changed in both versions; no claim that their 318-to-319 logic is equal'}
(W / 'DSP-reference-checks.json').write_text(json.dumps(summary, indent=2) + '\n')
