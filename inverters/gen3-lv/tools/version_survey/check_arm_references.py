"""Check branch bindings and list moved RAM operands of ARM shape candidates."""
import collections
import json
from common import WORK

inventory = json.loads((WORK / 'ARM-normalized.json').read_text())
results = {}
for pair, rows in inventory.items():
    mapping = {r['old']: r['candidates'][0] for r in rows if r['status'] == 'unique_shape'}
    counts = collections.Counter()
    mismatches, ram_changes = [], []
    for row in rows:
        references = row.get('reference_pairs', [])
        changes = [r for r in references if r['kind'] == 'ram' and r['old'] != r['new']]
        if changes:
            ram_changes.append({'name': row['name'], 'references': changes})
        for ref in references:
            if ref['kind'] != 'branch':
                continue
            expected = mapping.get(ref['old'])
            if expected is None:
                counts['unresolved_branch'] += 1
            elif expected == ref['new']:
                counts['consistent_branch'] += 1
            else:
                counts['different_branch_binding'] += 1
                mismatches.append({'name': row['name'], **ref, 'expected': expected})
    results[pair] = {'counts': dict(counts), 'branch_mismatches': mismatches,
                     'changed_ram_operands': ram_changes}
    print(pair, dict(counts), 'changed RAM functions', len(ram_changes))
(WORK / 'ARM-reference-checks.json').write_text(json.dumps(results, indent=2) + '\n')
