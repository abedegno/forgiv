"""Publish text-only candidate inventories and diagnostics from ignored output.

No disassembly, image bytes, compiler paths or target objects are published.
"""
import collections
import hashlib
import json
import re
from common import ROOT, WORK

DEST = ROOT / 'inverters/gen3-lv/docs/version-survey'


def read(name):
    return json.loads((WORK / name).read_text())


def write(name, data):
    (DEST / name).write_text(json.dumps(data, indent=2) + '\n')


sources = {}
for processor in ('a316', 'd316'):
    definitions = {}
    tree = ROOT / 'inverters/gen3-lv' / processor
    for file in sorted((tree / 'src').rglob('*.c')):
        if 'include' in file.parts:
            continue
        text = file.read_text()
        for name in re.findall(r'(?m)^[ \t]*(?:[A-Za-z_]\w*[ \t*]+)+([A-Za-z_]\w*)[ \t]*\([^;{}]*\)\s*\{', text):
            if name not in ('if', 'while', 'for', 'switch'):
                definitions.setdefault(name, str(file.relative_to(tree)))
    sources[processor] = definitions

report = ['# Named candidate lists', '',
          'No-hit rows need investigation; they are not a count of source logic changes.',
          'Source links identify the existing 316 definitions, not accepted newer ports.', '',
          'Ambiguous and unique rows are retained in the full candidate JSON inventories.', '']
metadata = {'accepted_complete_firmware_ports': [], 'shape_counts': {}, 'sha256': {}}
for processor, kind in (('a316', 'ARM'), ('d316', 'DSP')):
    inventory = read(f'{kind}-normalized.json')
    compact = {}
    for pair, rows in inventory.items():
        compact[pair] = []
        for row in rows:
            result = {k: v for k, v in row.items() if k != 'reference_pairs'}
            if len(result.get('candidates', [])) > 8:
                result['candidate_count_at_least'] = len(result['candidates'])
                result['candidates'] = result['candidates'][:8]
                result['candidate_list_truncated'] = True
            source = sources[processor].get(row['name'])
            if source:
                result['baseline_source'] = f'{processor}/{source}'
            compact[pair].append(result)
        metadata['shape_counts'][f'{kind}_{pair}'] = dict(collections.Counter(r['status'] for r in rows))
        report += [f'## {kind} {pair.replace("_", " -> ")} no shape hit', '',
                   '| Name | Baseline address | Existing 316 source |',
                   '| --- | --- | --- |']
        for row in rows:
            if row['status'] != 'no_shape_match':
                continue
            source = sources[processor].get(row['name'])
            link = f'[{source}](../../{processor}/{source})' if source else 'unassigned'
            report.append(f'| `{row["name"]}` | {row["old"]:08X} | {link} |')
        report.append('')
    write(f'{kind}-shape-candidates.json', compact)

for name in ('ARM-reference-checks.json', 'DSP-reference-checks.json',
             'DSP-changed-windows.json', 'unit-proofs.json'):
    write(name, read(name))

for pair, result in read('DSP-reference-checks.json').items():
    report += [f'## DSP {pair.replace("_", " -> ")} changed RAM operands', '',
               ', '.join('`' + row['name'] + '`' for row in result['changed_ram_bindings']) or 'None in this subset.', '']

(DEST / 'function-lists.md').write_text('\n'.join(report))
inputs = [ROOT / f'firmware/gen3-lv/{prefix}{version}_{suffix}Store.bin'
          for prefix, suffix in (('A', 'ARM'), ('D', 'DSP')) for version in (316, 318, 319)]
inputs += [ROOT / f'inverters/gen3-lv/{p}/map/functions.tsv' for p in ('a316', 'd316')]
inputs += list((ROOT / 'inverters/gen3-lv/tools/version_survey').glob('*.py'))
for path in sorted(inputs):
    metadata['sha256'][str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
write('relocation-metadata.json', metadata)
print('Published candidate diagnostics; no full source port accepted')
