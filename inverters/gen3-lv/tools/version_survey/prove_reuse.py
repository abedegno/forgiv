"""Compile unchanged C units and compare complete placed sections, unmasked.

Use documented a316/d316 toolchain configuration. MDK-Lite suffices. This
creates an owned Wine prefix under build/; only its server is stopped at exit.
No complete 318/319 firmware port or coverage is accepted by this diagnostic.
"""
from pathlib import Path
import atexit
import dataclasses
import json
import os
import subprocess
import sys
import tempfile
from common import ROOT as R, WORK as W
sys.path.insert(0, str(R / 'inverters/gen3-lv/a316/tools'))
import a316env
import armtc
import objplace
sys.path.insert(0, str(R / 'inverters/gen3-lv/d316/tools'))
import d316
prefix = tempfile.mkdtemp(prefix='probe-wine-', dir=W)
os.environ['WINEPREFIX'] = prefix
os.environ['WINEDLLOVERRIDES'] = 'mscoree,mshtml='
os.environ['WINEDEBUG'] = '-all'

def stop_owned_server():
    subprocess.run(['wineserver', '-k'], env=os.environ, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
atexit.register(stop_owned_server)
subprocess.run(['wineserver', '-p'], check=True)
subprocess.run(['wine', 'cmd', '/c', 'exit'], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
cfg = a316env.load()
tc = armtc.toolchain(cfg)
d316.BUILD = str(W / 'dsp-work')
arm_work, dsp_work = (W / 'arm-probe', W / 'dsp-probe')
arm_work.mkdir(exist_ok=True)
dsp_work.mkdir(exist_ok=True)
for stem, source in [('panel_led', 'src/app/drivers/panel_led.c'), ('util', 'src/app/util.c')]:
    ok, log = objplace.compile_source(cfg, tc, str(R / 'inverters/gen3-lv/a316' / source), str(arm_work / (stem + '.o')))
    (arm_work / (stem + '-compile.log')).write_text(log)
    if not ok:
        raise RuntimeError('ARM compile failed: see ignored compile log')

class Runner:

    def run(self, args, cwd=None, check=True):
        result = subprocess.run(['wine'] + args, cwd=cwd, env=os.environ, stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=180)
        if check and result.returncode:
            raise RuntimeError('TI compiler failed; no source match accepted')
        return result
d316.compile_c(Runner(), str(R / 'inverters/gen3-lv/d316/src/app/util/rate_limit.c'), str(dsp_work), extra=['-Ooff'])
sys.path.insert(0, str(R / 'inverters/gen3-lv/a316/tools'))
import a316env, armtc, objplace
import csv, struct
cfg = a316env.load()
tc = armtc.toolchain(cfg)
o = objplace.read_object(tc, str(W / 'arm-probe/panel_led.o'))
idx = next((i for i, s in o.sections.items() if s.name == '.text'))
s = o.sections[idx]
mask = bytearray(b'\xff' * s.size)
for r in o.relocs[idx]:
    if r.tname != 'R_ARM_THM_CALL':
        raise ValueError(r.tname)
    mask[r.offset:r.offset + 4] = b'\x00' * 4
maps = {r['name']: int(r['addr'], 16) for r in csv.DictReader((R / 'inverters/gen3-lv/a316/map/functions.tsv').open(), delimiter='\t')}
report = {'ARM': {}, 'DSP': {}}
for v in (316, 318, 319):
    image = (R / f'firmware/gen3-lv/A{v}_ARMStore.bin').read_bytes()[4:]
    hits = []
    for k in range(0, len(image) - s.size + 1, 2):
        if all((a & c == b & c for a, b, c in zip(s.data, image[k:k + s.size], mask))):
            hits.append(k)
    if len(hits) != 1:
        if v != 319:
            raise AssertionError('panel baseline/318 unit no longer matches')
        report['ARM'][str(v)] = {'source': 'a316/src/app/drivers/panel_led.c', 'status': 'no_complete_object_match', 'candidate_count': len(hits), 'note': '319 panel functions start at halfword alignment; pool placement requires further work'}
        print('ARM', v, 'panel unit not exact as compiled', flush=True)
        continue
    base = 134299648 + hits[0]
    table = {}
    refs = []
    for r in o.relocs[idx]:
        h1, h2 = struct.unpack_from('<HH', image, hits[0] + r.offset)
        target = base + r.offset + 4 + objplace._bl_get(h1, h2)
        table[r.sname] = (target | 1, 'T')
        refs.append({'symbol': r.sname, 'target': target})
    linked, errors, count = objplace.link(o, {idx: base}, table)
    assert not errors, errors
    assert linked[idx] == image[hits[0]:hits[0] + s.size], v
    for ref in refs:
        assert table[ref['symbol']][0] & ~1 == ref['target']
        if v == 316:
            assert ref['target'] == maps[ref['symbol']], ref
        else:
            raw = json.loads((R / f'build/gen3-lv-version-survey/ARM{v}-exact-candidates.json').read_text())
            evidence = next((x for x in raw if x['name'] == ref['symbol']), None)
            if evidence:
                assert int(evidence['candidate_addr'], 16) == ref['target'], ref
            else:
                row = next((x for x in csv.DictReader((R / 'inverters/gen3-lv/a316/map/functions.tsv').open(), delimiter='\t') if x['name'] == ref['symbol']))
                oldimage = (R / 'firmware/gen3-lv/A316_ARMStore.bin').read_bytes()[4:]
                length = int(row['size'], 16)
                oldoff = maps[ref['symbol']] - 134299648
                newoff = ref['target'] - 134299648
                assert oldimage[oldoff:oldoff + length] == image[newoff:newoff + length], ref
    report['ARM'][str(v)] = {'source': 'a316/src/app/drivers/panel_led.c', 'base': base, 'bytes': s.size, 'functions': 8, 'relocations': count, 'different_bytes': 0, 'external_symbols': {n: val for n, (val, kind) in table.items()}, 'method': 'objplace applied every real object relocation; callee bodies checked against complete 316 map rows (unique raw candidates where available; short GPIO leaves checked at decoded targets)'}
    print('ARM', v, hex(base), s.size, 'bytes exact, external bindings checked', flush=True)
sys.path.insert(0, str(R / 'inverters/gen3-lv/d316/tools'))
import d316
c = d316.cfg()
d316.BUILD = str(W / 'dsp-work')
rows = json.loads((W / 'DSP-normalized.json').read_text())
for v in (316, 318, 319):
    load = 4141856 if v == 316 else next((x['candidates'][0] for x in rows[str(v)] if x['name'] == 'rate_limit'))
    loadstart = 4135863 if v == 316 else next((r['new'] for x in rows[str(v)] if x['name'] == 'init_system' for r in x['reference_pairs'] if r['old'] == 4135863))
    run = 32768 + load - loadstart
    work = W / 'dsp-probe'
    cmd = work / f'D{v}.cmd'
    out = work / f'D{v}.out'
    cmd.write_text(f'MEMORY {{ PROBE : origin = 0x{run:x}, length = 0x2e }}\nSECTIONS {{ ramfuncs:_rate_limit : > PROBE }}\n')
    result = d316.run([d316.native('lnk2000'), '--disable_auto_rts', '--disable_clink', str(work / 'rate_limit.obj'), str(cmd), '-o', str(out)], check=False)
    (work / f'D{v}-link.log').write_text(result.stdout + result.stderr)
    assert result.returncode == 0, 'DSP link failed'
    segs, obj = d316.out_load_words(str(out))
    section = next((s for s in obj.sections.values() if s.name == 'ramfuncs:_rate_limit'))
    assert section.paddr == run
    image = (R / f'firmware/gen3-lv/D{v}_DSPStore.bin').read_bytes()
    target = struct.unpack_from('>46H', image, (load - 4096000) * 2)
    assert list(target) == section.data
    report['DSP'][str(v)] = {'source': 'd316/src/app/util/rate_limit.c', 'load': load, 'run': run, 'words': 46, 'relocations': 0, 'different_words': 0, 'method': 'native lnk2000 at RAM run address inferred from startup copy source; all 46 linked words compared unmasked at flash load address'}
    print('DSP', v, hex(load), hex(run), '46 words exact', flush=True)
(W / 'unit-proofs.json').write_text(json.dumps(report, indent=2) + '\n')
sys.path.insert(0, str(R / 'inverters/gen3-lv/a316/tools'))
import a316env, armtc, objplace
cfg = a316env.load()
tc = armtc.toolchain(cfg)
work = W / 'arm-probe'
src = R / 'inverters/gen3-lv/a316/src/app/util.c'
path = work / 'util.o'
ok, log = objplace.compile_source(cfg, tc, str(src), str(path))
(work / 'util-compile.log').write_text(log)
assert ok
o = objplace.read_object(tc, str(path))
idx = next((i for i, s in o.sections.items() if s.name == '.text'))
sec = o.sections[idx]
assert not o.relocs.get(idx)
p = json.loads((W / 'unit-proofs.json').read_text())
p['ARM_util'] = {}
for v in (316, 318, 319):
    image = (R / f'firmware/gen3-lv/A{v}_ARMStore.bin').read_bytes()[4:]
    hits = []
    pos = 0
    while True:
        k = image.find(sec.data, pos)
        if k < 0:
            break
        if k % 2 == 0:
            hits.append(k)
        pos = k + 1
    assert len(hits) == 1, (v, len(hits), sec.size)
    base = 134299648 + hits[0]
    linked, errors, count = objplace.link(o, {idx: base}, {})
    assert not errors and linked[idx] == image[hits[0]:hits[0] + sec.size]
    p['ARM_util'][str(v)] = {'source': 'a316/src/app/util.c', 'base': base, 'bytes': sec.size, 'functions': 6, 'relocations': count, 'different_bytes': 0, 'method': 'complete compiled .text compared unmasked after objplace; no external relocations'}
    print('ARM util', v, hex(base), sec.size, 'exact', flush=True)
(W / 'unit-proofs.json').write_text(json.dumps(p, indent=2) + '\n')

# Keep provenance about source and object bytes without retaining either object
# or any machine-specific path in the committed evidence.
import hashlib
for group, versions in p.items():
    for version, entry in versions.items():
        source = R / 'inverters/gen3-lv' / entry['source']
        entry['source_sha256'] = hashlib.sha256(source.read_bytes()).hexdigest()
        if entry.get('status') != 'no_complete_object_match':
            artifact = W / ('dsp-probe/rate_limit.obj' if group == 'DSP' else
                            'arm-probe/util.o' if group == 'ARM_util' else
                            'arm-probe/panel_led.o')
            entry['object_sha256'] = hashlib.sha256(artifact.read_bytes()).hexdigest()
(W / 'unit-proofs.json').write_text(json.dumps(p, indent=2) + '\n')
