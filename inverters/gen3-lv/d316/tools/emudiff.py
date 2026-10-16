"""Differential test of a modding-build image against the original: the same calls of a set of
pure functions run in both images in Ghidra's p-code emulator (the ghidra-tms320c28x processor
module), and the results must agree. Where the answer is plain arithmetic it is also checked
against Python.

    python3 tools/emudiff.py [IMAGE] [-v]

IMAGE is a modding-build image (default build/mod/DSPStore.bin); its addresses come from the
linker's XML beside it (DSPStore.xml, written by tools/modbuild.py). The original's come from
the map. Each image's ramfuncs are copied to their run addresses first, as init_system does.

Needs Ghidra 12 with the ghidra-tms320c28x extension installed and a JDK 21:
GHIDRA_INSTALL_DIR (default: Homebrew's ghidra) and JAVA_HOME (default: Homebrew's openjdk@21).
The emulator models the CPU and memory only (no peripherals, interrupts or flash wait states),
so only functions that touch nothing else are tested. Exit status 0 when every call agrees."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d316  # noqa: E402  (first: it loads the standard library's dis, which tools/dis.py hides)
import argparse, glob, random, shutil, struct, subprocess, time  # noqa: E402
import xml.etree.ElementTree as ET  # noqa: E402

EMU = os.path.join(d316.BUILD, 'emu')
BUF = 0xC000            # RAM for buffers and pointer arguments
STACK = 0x0500          # the emulator's SP (EmuDiff.java); stack arguments go below it


def f2w(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def w2f(w):
    return struct.unpack('<f', struct.pack('<I', w & 0xFFFFFFFF))[0]


def crc16_modbus(ws):
    crc = 0xFFFF
    for w in ws:
        crc ^= w & 0xFF
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def cases(rng):
    """(function, regs, mem, outs, python's answer or None, label); regs are register: value"""
    out = []
    msgs = [[0x01, 0x03, 0x00, 0x00, 0x00, 0x0A], [0x11, 0x06, 0x00, 0x01, 0x00, 0x03], []] + \
           [[rng.randrange(256) for _ in range(rng.randrange(1, 40))] for _ in range(6)]
    for m in msgs:
        out.append(('CRC16', dict(XAR4=BUF, AL=len(m)), {BUF: m}, ['AL'], {'AL': crc16_modbus(m)}, 'len %d' % len(m)))
    fl = [0.0, 1.5, -2.25, 1e6, -1e-3, 3.14159, 42.0, -7.5]
    for _ in range(6):
        a, b, c = (rng.choice(fl + [rng.uniform(-1000, 1000)]) for _ in range(3))
        r = dict(R0H=f2w(a), R1H=f2w(b), R2H=f2w(c))
        out.append(('max3f', r, {}, ['R0H'], {'R0H': f2w(max(w2f(f2w(a)), w2f(f2w(b)), w2f(f2w(c))))}, '%g %g %g' % (a, b, c)))
        out.append(('min3f', r, {}, ['R0H'], {'R0H': f2w(min(w2f(f2w(a)), w2f(f2w(b)), w2f(f2w(c))))}, '%g %g %g' % (a, b, c)))
    for x, lim in ((5, 10), (50, 10), (-50, 10), (-10, 10), (0, 0), (32767, 1000), (-32768, 32767)):
        want = lim if x > lim else -lim if x < -lim else x
        out.append(('limit_sym', dict(AL=x & 0xFFFF, AH=lim & 0xFFFF), {}, ['AL'], {'AL': want & 0xFFFF}, '%d, %d' % (x, lim)))
    for a_, b, c in ((10, 5, 20), (10, 15, 20), (10, 25, 20), (0, 0, 0), (100, 65535, 200)):
        want = b if a_ <= b <= c else a_ if b < a_ else c
        out.append(('clampu', dict(AL=a_, AH=b, AR4=c), {}, ['AL'], {'AL': want}, '%d %d %d' % (a_, b, c)))
    for x in (0, 1, -1, 123456, -123456, -0x7FFFFFFF):
        out.append(('labs32', dict(ACC=x & 0xFFFFFFFF), {}, ['ACC'], {'ACC': abs(x) & 0xFFFFFFFF}, '%d' % x))
    for x, d in ((1000, 7), (0xFFFF0, 0x10), (5, 0), (123456, 1000), (0xFFFFF, 0x20)):
        want = 0 if d == 0 else (x // d) & 0xFFFF
        # (no Python answer for the two divisions by RPT || SUBCUL: the module's emulator does not
        # leave the quotient in P there, so both images give the same wrong number; they are
        # compared with each other only)
        out.append(('udiv32_16', dict(ACC=x, AR4=d), {}, ['AL'], None, '%d / %d' % (x, d)))
    for x, d in ((0, 1), (1000000, 1), (50000000, 3), (0xFFFFFFFF, 1), (4000000, 0), (9999, 7)):
        out.append(('isqrt_div', dict(ACC=x, AR4=d), {}, ['ACC'], None, 'sqrt(%d / %d)' % (x, d)))
    for x in (0, 718, 719, 1024, 1126, 1127, 65535):
        out.append(('cal_gain_or_unity', dict(AL=x), {}, ['AL'], {'AL': 0x400 if x < 719 or x > 1126 else x}, '%d' % x))
    for d in range(16):
        out.append(('hexdigit', dict(AL=d), {}, ['AL'], {'AL': d + 48 if d < 10 else d + 55}, '%d' % d))
    for buf, lo, hi in (([1, 2, 3, 4, 5], 0, 4), ([0xFFFF] * 4, 0, 3), ([0, 1, 2], 0, 2), ([7, 9], 1, 1)):
        s = sum(buf[i] ^ i for i in range(lo, hi + 1))
        out.append(('checksum_xor', dict(XAR4=BUF, AL=lo, AH=hi), {BUF: buf}, ['AL'], {'AL': 0xFFFF if s == 0 or s > 0xFFFF else s}, '%s' % buf))
    for a_, b, c in ((1, 2, 3), (3, 1, 2), (2, 3, 1), (5, 5, 1), (0xFFFFFFFF, 0, 7)):
        # a in ACC, b and c on the stack below SP (the caller's outgoing argument area)
        mem = {STACK - 4: [b & 0xFFFF, b >> 16], STACK - 6: [c & 0xFFFF, c >> 16]}
        out.append(('median3ul', dict(ACC=a_), mem, ['ACC'], {'ACC': sorted([a_, b, c])[1]}, '%d %d %d' % (a_, b, c)))
    for x, mx, mn in ((5, 10, 0), (50, 10, 0), (0, 100, 10), (0xFFFF, 0xFFFE, 0)):
        want = min(x, mx); want = max(want, mn)
        out.append(('limitu', dict(XAR4=BUF, AL=mx, AH=mn), {BUF: [x]}, ['mem %d 1' % BUF], {'mem': [want]}, '%d in %d..%d' % (x, mn, mx)))
    for x, mx, mn in ((5.0, 10.0, 0.0), (50.0, 10.0, -1.0), (-3.0, 1.0, -2.5)):
        want = f2w(max(min(x, mx), mn))
        out.append(('limitf', dict(XAR4=BUF, R0H=f2w(mx), R1H=f2w(mn)), {BUF: [f2w(x) & 0xFFFF, f2w(x) >> 16]},
                    ['mem %d 2' % BUF], {'mem': [want & 0xFFFF, want >> 16]}, '%g in %g..%g' % (x, mn, mx)))
    # a ramfunc (run from RAM): rate_limit(&x, ref, up, lo)
    for x, ref, up, lo in ((10.0, 0.0, 1.0, -1.0), (0.5, 0.0, 1.0, -1.0), (-5.0, 2.0, 0.5, -0.5)):
        out.append(('rate_limit', dict(XAR4=BUF, R0H=f2w(ref), R1H=f2w(up), R2H=f2w(lo)), {BUF: [f2w(x) & 0xFFFF, f2w(x) >> 16]},
                    ['mem %d 2' % BUF], None, 'x %g ref %g step %g..%g' % (x, ref, lo, up)))
    # the runtime library: float division, square root, exp (expf reads its constants in .econst)
    for a_, b in ((1.0, 3.0), (-10.0, 4.0), (1e6, 7.0), (0.0, 5.0)):
        out.append(('FS$$DIV', dict(R0H=f2w(a_), R1H=f2w(b)), {}, ['R0H'], None, '%g / %g' % (a_, b)))
    for x in (0.0, 2.0, 1e6, 0.25):
        out.append(('sqrtf', dict(R0H=f2w(x)), {}, ['R0H'], None, 'sqrt %g' % x))
    for x in (0.0, 1.0, -1.0, 5.5, -20.0):
        out.append(('expf', dict(R0H=f2w(x)), {}, ['R0H'], None, 'exp %g' % x))
    return out


def orig_addrs():
    out = {}
    for r in d316.functions():
        out[r['name']] = d316.run_addr(r)
    ram = (d316.resolve('RamfuncsLoadStart'), d316.resolve('RamfuncsRunStart'), d316.resolve('RamfuncsLoadSize'))
    return out, ram


def mod_addrs(xmlpath):
    r = ET.parse(xmlpath).getroot()
    syms = {s.findtext('name'): int(s.findtext('value'), 16) for s in r.find('symbol_table')}
    out = {}
    for n, v in syms.items(): out[n[1:] if n.startswith('_') else n] = v
    for n, v in syms.items():
        if '$$' in n: out[n] = v
    ram = (syms['_RamfuncsLoadStart'], syms['_RamfuncsRunStart'], syms['_RamfuncsLoadSize'])
    return out, ram


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('image', nargs='?', default=os.path.join(d316.BUILD, 'mod', 'DSPStore.bin'))
    ap.add_argument('--seed', type=int, default=316)
    ap.add_argument('-v', action='store_true')
    a = ap.parse_args()
    t0 = time.time()
    ghidra = os.environ.get('GHIDRA_INSTALL_DIR') or next(iter(sorted(glob.glob('/opt/homebrew/Cellar/ghidra/*/libexec'))[-1:]), '')
    java = os.environ.get('JAVA_HOME') or '/opt/homebrew/opt/openjdk@21'
    headless = os.path.join(ghidra, 'support', 'analyzeHeadless')
    if not os.path.exists(headless):
        print('no Ghidra (set GHIDRA_INSTALL_DIR)'); sys.exit(2)
    xmlpath = os.path.splitext(a.image)[0] + '.xml'
    oa, oram = orig_addrs()
    ma, mram = mod_addrs(xmlpath)
    calls = cases(random.Random(a.seed))
    shutil.rmtree(EMU, ignore_errors=True); os.makedirs(EMU)
    spec = []
    orig_bin = os.path.join(d316.ROOT, d316.cfg()['image']['bin'])
    for label, path, addrs, ram in (('orig', orig_bin, oa, oram), ('mod', os.path.abspath(a.image), ma, mram)):
        spec.append('IMAGE %s %s 0x%X 0x%X 0x%X' % ((label, path) + ram))
        for i, (fn, regs, mem, outs, want, what) in enumerate(calls):
            spec.append('CASE %d 0x%X' % (i, addrs[fn]))
            for k, v in mem.items(): spec.append('MEM 0x%X %s' % (k, ','.join('0x%X' % w for w in v) or '0'))
            for k, v in regs.items(): spec.append('REG %s 0x%X' % (k, v))
            for o in outs: spec.append('OUT ' + o)
            spec.append('END')
    open(os.path.join(EMU, 'spec.txt'), 'w').write('\n'.join(spec) + '\n')
    open(os.path.join(EMU, 'dummy.bin'), 'wb').write(b'\x06\x00' * 8)       # LRETR: an import target
    env = dict(os.environ, JAVA_HOME=java, GHIDRA_INSTALL_DIR=ghidra)
    env['PATH'] = os.path.join(java, 'bin') + os.pathsep + env.get('PATH', '')
    os.makedirs(os.path.join(EMU, 'proj'))
    p = subprocess.run([headless, os.path.join(EMU, 'proj'), 'emudiff', '-import', os.path.join(EMU, 'dummy.bin'),
                        '-processor', 'TMS320C28x:LE:32:default', '-noanalysis', '-deleteProject',
                        '-scriptPath', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'emu'),
                        '-postScript', 'EmuDiff.java', os.path.join(EMU, 'spec.txt'), os.path.join(EMU, 'out.txt')],
                       capture_output=True, text=True, env=env, timeout=1800)
    open(os.path.join(EMU, 'ghidra.log'), 'w').write(p.stdout + p.stderr)
    if 'EmuDiff.java> done' not in p.stdout:
        print('Ghidra did not run the script; build/emu/ghidra.log has its output'); sys.exit(2)
    res = {}
    for line in open(os.path.join(EMU, 'out.txt')):
        label, i, rest = line.rstrip('\n').split(' ', 2)
        res[(label, int(i))] = rest
    bad, agree, checked, pyok = [], 0, 0, 0
    for i, (fn, regs, mem, outs, want, what) in enumerate(calls):
        o, m = res.get(('orig', i), 'missing'), res.get(('mod', i), 'missing')
        # the step counts may differ by nothing; the results must be the same
        ok = o == m and not o.startswith('ERROR')
        pv = ''
        if want: checked += 1
        if ok and want:
            got = dict(kv.split('=', 1) for kv in o.split()[1:])
            for k, v in want.items():
                if k == 'mem':
                    g = [int(x, 16) for x in next(val for kk, val in got.items() if kk.startswith('mem')).split(',')]
                    ok_k = g == v
                else:
                    ok_k = int(got[k], 16) == v
                if not ok_k: ok = False; pv = ' (Python: %s)' % want
            if ok: pyok += 1
        if o == m and not o.startswith('ERROR'): agree += 1
        if not ok: bad.append('%s(%s): original %s, mod %s%s' % (fn, what, o, m, pv))
        if a.v: print('  %-18s %-24s %s %s' % (fn, what, 'ok  ' if ok else 'FAIL', m))
    fns = sorted({c[0] for c in calls})
    print('emudiff: %d calls of %d functions, original image and %s, in Ghidra\'s emulator' % (len(calls), len(fns), os.path.relpath(a.image, d316.ROOT)))
    print('  %s' % ', '.join(fns))
    print('  %d of %d calls give the same result in both images; %d of %d answers also agree with Python' % (agree, len(calls), pyok, checked))
    for b in bad: print('  FAIL ' + b)
    print('%s  (%.1f s)' % ('PASS' if not bad else 'FAIL', time.time() - t0))
    sys.exit(0 if not bad else 1)


if __name__ == '__main__':
    main()
