"""Export local instruction inventories. Requires Exhume, Ghidra and native TI tools.

Configure a316/d316 toolchain.local.toml or their documented environment
variables first. No compiler or image is copied into the source tree.
"""
import csv
import dataclasses  # Load stdlib inspect before the target tools' dis.py can shadow dis.
import json
import subprocess
import sys
from common import ROOT, WORK

sys.path.insert(0, str(ROOT / 'inverters/gen3-lv/a316/tools'))
import a316env
import armmapgen
import ehex
sys.path.insert(0, str(ROOT / 'inverters/gen3-lv/d316/tools'))
import d316

d316.BUILD = str(WORK / 'dsp-work')
for version in (316, 318, 319):
    data = (ROOT / f'firmware/gen3-lv/D{version}_DSPStore.bin').read_bytes()
    words = [int.from_bytes(data[k:k+2], 'big') for k in range(0, len(data), 2)]
    rows = d316.disasm(words, 0x3E8000, cache_key=f'D{version}')
    (WORK / f'D{version}-instructions.json').write_text(json.dumps(
        [[address, len(raw), text] for address, raw, text in rows]))

baseline = list(csv.DictReader((ROOT / 'inverters/gen3-lv/a316/map/functions.tsv').open(), delimiter='\t'))
old_image = ehex.Image(str(ROOT / 'firmware/gen3-lv/A316.hex'))
for version in (316, 318, 319):
    image = ehex.Image(str(ROOT / f'firmware/gen3-lv/A{version}.hex'))
    seeds = armmapgen.seeds(image)
    if version == 316:
        seeds |= {int(r['addr'], 16) for r in baseline}
    else:
        candidates = json.loads((ROOT / f'build/gen3-lv-version-survey/ARM{version}-exact-candidates.json').read_text())
        seeds |= {int(r['candidate_addr'], 16) for r in candidates}
    for offset in range(0, len(image.data)-3, 2):
        pointer = int.from_bytes(image.data[offset:offset+4], 'little')
        if pointer & 1 and image.base <= pointer-1 < image.end-4:
            instruction = armmapgen.first_insn(image, pointer-1)
            if instruction.startswith('push') and 'lr' in instruction:
                seeds.add(pointer-1)
    functions = armmapgen.run_ghidra(image, seeds, str(WORK / f'ghidra-A{version}'))
    if version != 316:
        seeds = set(functions)
        for row in baseline:
            address, size = int(row['addr'], 16), int(row['size'], 16)
            if size < 4:
                continue
            pattern = old_image.bytes(address, size)
            hits, position = [], 0
            while True:
                offset = image.data.find(pattern, position)
                if offset < 0:
                    break
                if offset % 2 == 0:
                    hits.append(offset)
                if len(hits) > 1:
                    break
                position = offset + 1
            if len(hits) == 1:
                seeds.add(image.base + hits[0])
        # These images use a scatterload form the generic region-table detector
        # does not recognize. Empty regions are sufficient for gap code seeds;
        # no RAM/data coverage claim is made by this inventory.
        _, _, _, gaps = armmapgen.build_map(image, functions, [])
        seeds |= gaps
        functions = armmapgen.run_ghidra(image, seeds, str(WORK / f'ghidra-A{version}-refined'))
        armmapgen.build_map(image, functions, [])
    (WORK / f'A{version}-functions.json').write_text(json.dumps(functions))
    print('ARM', version, len(functions), 'function candidates', flush=True)

# A first signature pass supplies neighboring layout anchors for dead code.
subprocess.run([sys.executable, str(__file__).replace('export_inventories.py', 'normalize_arm.py')], check=True)
normalized = json.loads((WORK / 'ARM-normalized.json').read_text())
for version in (318, 319):
    image = ehex.Image(str(ROOT / f'firmware/gen3-lv/A{version}.hex'))
    destination = WORK / f'A{version}-functions.json'
    seeds = {int(k) for k in json.loads(destination.read_text())}
    rows = normalized[f'316_{version}']
    anchors = [r for r in rows if r['status'] == 'unique_shape']
    for row in rows:
        if row['status'] != 'no_shape_match':
            continue
        for anchor in sorted(anchors, key=lambda r: abs(r['old']-row['old']))[:4]:
            address = row['old'] + anchor['candidates'][0] - anchor['old']
            if image.base <= address < image.end-4 and image.bytes(address, 2) == old_image.bytes(row['old'], 2):
                seeds.add(address)
    functions = armmapgen.run_ghidra(image, seeds, str(WORK / f'ghidra-A{version}-predicted'))
    destination.write_text(json.dumps(functions))
    print('ARM', version, len(functions), 'final function candidates', flush=True)
