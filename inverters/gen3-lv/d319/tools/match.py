"""Compile a C file (or assemble a TI assembly file, SRC.asm) and compare each function with the
image.

    python3 tools/match.py SRC.c|SRC.asm [FUNC ...] [--cgt DIR] [-v] [--keep]

The file is compiled (an .asm file is assembled by the same cl2000) with the flags in
toolchain.toml (a line `// cflags: ...` in the file adds flags) and --gen_func_subsections=on,
so each C function is a section of its own (.text:_name,
.switch:_name for its switch tables, .econst:_name for each const object). The object is then
linked with lnk2000, each section at the address the map gives its name:

  - a function at its map address (a ramfunc at its run address; its words are compared with
    the image at its load address);
  - its switch table at the map's sw_<function> row;
  - a const object at its data row (it is compared too);
  - a section with no symbol in the map (an assembly file's codestart, csmpasswds, csm_rsvd) at
    the row whose notes say section=NAME (compared too);
  - .ebss where the map puts the variables it defines (a variable named var_D8E2, or with a
    name in map/data.tsv);
  - every symbol the file uses but does not define at its map address. A name the map does
    not have resolves only if it says its address: sub_3EAC5D, var_D8E2, c_3F3E78 ...

So relocated words are not masked away: each relocation is filled in by the linker and must come
out as the image's word, which checks that its target is the address the image uses. A target
that does not resolve is reported as UNRESOLVED with the address the image's words point to.

Prints EXACT for each function that matches word for word, or the first differences with both
disassemblies side by side. Exit status 0 if every function compared is EXACT."""
import argparse, os, re, shutil, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d319

PARK = 0x200000            # where sections not compared are linked (outside the device's memory)
PARK_DATA = 0x300000


def cflags_of(src):
    extra = []
    for line in open(src, encoding='latin1'):
        m = re.match(r'\s*//\s*cflags:\s*(.*)$', line)
        if m: extra += m.group(1).split()
    return extra


def cgt_of(src):
    """a file's own compiler, from a '// cgt: VERSION' line (a directory under toolchain.toml's
    [compiler] others, e.g. 16.9.4.LTS); None for the default compiler"""
    import glob
    for line in open(src, encoding='latin1'):
        m = re.match(r'\s*//\s*cgt:\s*(\S+)', line)
        if m:
            pat = os.path.expanduser(d319.cfg()['compiler']['others'])
            for exe in glob.glob(pat):
                home = os.path.dirname(os.path.dirname(exe))
                if os.path.basename(os.path.dirname(home)) == m.group(1) or os.path.basename(home).endswith(m.group(1)):
                    return home
            raise d319.ToolError('no compiler %s under %s' % (m.group(1), pat))
    return None


def map_rows():
    f = {r['name']: r for r in d319.functions()}
    d = {r['name']: r for r in d319.data()}
    return f, d


def cname(sym):
    return sym[1:] if sym.startswith('_') else sym


class Result:
    def __init__(self, name, addr, run, size, kind):
        self.name, self.addr, self.run, self.size, self.kind = name, addr, run, size, kind
        self.ours, self.diffs, self.relocs, self.problems, self.plain_diffs = [], [], [], [], []

    @property
    def exact(self):
        return not self.diffs and not self.problems and len(self.ours) == self.size


def decode(typ, ws, k):
    """the address a relocated field holds in the image's words ws at k"""
    try:
        if typ == 'R_REL22': return ((ws[k] & 0x3F) << 16) | ws[k + 1]
        if typ in ('R_RELLONG', 'R_ANKPTR'): return ws[k] | ws[k + 1] << 16
        if typ in ('R_PARTMS16',): return ws[k] << 6
        if typ in ('R_PARTLS6',): return ws[k] & 0x3F
        return ws[k]
    except IndexError:
        return None


def link(obj, outdir, placements, symdefs):
    """lnk2000 (native) obj with sections at addresses; returns the .out path."""
    cmd = os.path.join(outdir, 'm.cmd')
    with open(cmd, 'w') as f:
        f.write('-a\n-o %s\n-m %s\n' % (os.path.join(outdir, 'm.out'), os.path.join(outdir, 'm.map')))
        f.write('--disable_auto_rts\n--disable_clink\n')
        f.write('SECTIONS\n{\n')
        for sec, addr, page in placements:
            f.write('    %s : load = 0x%X, PAGE = %d\n' % (quote(sec), addr, page))
        f.write('}\n')
        for s, v in sorted(symdefs.items()):
            f.write('%s = 0x%X;\n' % (s, v))
    p = d319.run([d319.native('lnk2000'), obj, cmd], cwd=outdir, check=False)
    out = os.path.join(outdir, 'm.out')
    if p.returncode or not os.path.exists(out):
        raise d319.ToolError('lnk2000 failed:\n' + p.stdout + p.stderr)
    return out, p.stdout + p.stderr


def quote(sec):
    return '"%s"' % sec if re.search(r'[^\w.$]', sec) else sec


def plan(o, frows, drows):
    """placements and symbol definitions for object o. Returns (placements, symdefs, targets, notes)
    targets: list of (Result, section name, offset in section, compare at load address)"""
    placements, targets, notes = [], [], []
    defined = o.defined()
    statics = {s['name']: (s['section'], s['value']) for s in o.symbols
               if s['sclass'] == 'C_STAT' and s['kind'] == 'defined' and s['section'] and not s['name'].startswith(('$', '.'))}
    allsyms = dict(statics); allsyms.update(defined)
    bysec = {}
    for n, (sec, v) in allsyms.items(): bysec.setdefault(sec, []).append((v, n))
    page_park = [PARK, PARK_DATA]
    for name, sec in o.sections.items():
        if sec.kind not in ('text', 'data', 'bss') or name.startswith('.debug') or name in ('$build.attributes',):
            continue
        if sec.size == 0: continue
        syms = sorted(bysec.get(name, []))
        base, page = None, 0 if sec.kind == 'text' or name.startswith(('.econst', '.switch', '.cinit')) else 1
        if name.startswith('.switch:'):
            owner = cname(name.split(':', 1)[1])
            row = drows.get('sw_' + owner)
            if row:
                base = row['addr']
                r = Result('sw_' + owner, row['addr'], row['addr'], row['size'], 'switch')
                targets.append((r, name, 0, row['addr']))
            else:
                notes.append('%s: no sw_%s row in map/data.tsv' % (name, owner))
        elif name.startswith('.cinit'):
            pass
        else:
            for v, n in syms:
                c = cname(n)
                row = frows.get(c) if sec.kind == 'text' else drows.get(c)
                at = None
                if row:
                    at, size, load, kind = d319.run_addr(row), row['size'], row['addr'], row['kind']
                elif d319.resolve(n) is not None:
                    at = d319.resolve(n); size, load, kind = None, at, 'data' if sec.kind != 'text' else 'c'
                if at is None: continue
                b = at - v
                if base is None: base = b
                elif base != b:
                    notes.append('%s: %s wants the section at 0x%X, another symbol at 0x%X' % (name, n, b, base))
                if sec.kind == 'bss': continue
                if size is None:
                    nxt = next((w for w, _ in syms if w > v), sec.size)
                    size = nxt - v
                r = Result(c, load, at, size, kind)
                targets.append((r, name, v, load))
        if base is None and sec.kind in ('text', 'data') and not name.startswith(('.cinit', '.switch')):
            # a section with no symbol the map knows (TI's assembly: codestart, csmpasswds,
            # csm_rsvd) goes where a map row's notes say section=NAME, in flash
            row = next((r for r in list(frows.values()) + list(drows.values())
                        if d319.note(r, 'section') == name and not d319.note(r, 'member')), None)
            if row:
                base, page = row['addr'], 0
                targets.append((Result(row['name'], row['addr'], row['addr'], row['size'], row['kind']), name, 0, row['addr']))
        if base is None and sec.kind in ('bss', 'data') and not name.startswith(('.cinit', '.switch')):
            # file-local data (statics) has no name in the map: take the address the image's
            # words give the section, where this file's placed functions use it
            vals = []
            for tname, tsec in o.sections.items():
                if tsec.kind != 'text': continue
                for v, n in sorted(bysec.get(tname, [])):
                    row = frows.get(cname(n))
                    if row is None: continue
                    for sym, got in d319.derive_bases(tsec, row['addr'] - v).items():
                        if sym == name: vals += got
                    break
            if vals:
                common = max(set(vals), key=vals.count)
                base = common
                notes.append('%s: placed at 0x%X from the image\'s words (%d of %d uses agree)'
                             % (name, base, vals.count(common), len(vals)))
        if base is not None and d319.BASE <= base < d319.END:
            page = 0            # flash is program memory (csm_rsvd: a data section named like its row)
        if base is None:
            base = page_park[page]
            page_park[page] += sec.size + 64
            if sec.kind == 'text' or name.startswith(('.econst', '.switch')):
                notes.append('%s: no map address (not compared)' % name)
        placements.append((name, base, page))
    # undefined symbols
    symdefs, unresolved = {}, []
    for n in o.undefined():
        v = d319.resolve(n)
        if v is None:
            unresolved.append(n); v = 0
        symdefs[n] = v
    return placements, symdefs, targets, notes, unresolved


def compare(o, outpath, targets, unresolved):
    segs, lo = d319.out_load_words(outpath)
    linked = {}
    for name, s in lo.sections.items():
        if s.data: linked[name] = s.data
    results = []
    for r, secname, off, load in targets:
        data = linked.get(secname, [])
        sec = o.sections[secname]
        r.ours = data[off:off + r.size]
        img = d319.words(load, r.size) if d319.BASE <= load and load + r.size <= d319.END else []
        if len(img) != r.size: r.problems.append('outside the image')
        for k in range(min(len(r.ours), len(img))):
            if r.ours[k] != img[k]: r.diffs.append(k)
        if len(r.ours) != r.size: r.problems.append('size %d, map %d' % (len(r.ours), r.size))
        bits = d319.reloc_masks(sec)[off:off + r.size]
        # where words other than relocated bits differ the code itself differs, and what the
        # relocations hold there says nothing
        r.plain_diffs = [k for k in r.diffs if k < len(bits) and (r.ours[k] ^ img[k]) & ~bits[k] & 0xFFFF]
        for roff, typ, sym in sec.relocs:
            if typ in d319.RELOC_NOFIELD or not off <= roff < off + r.size: continue
            k = roff - off
            fields = d319.RELOC_FIELDS.get(typ, [(0, 0xFFFF), (1, 0xFFFF)])
            ok = all(k + j < len(r.ours) and k + j < len(img) and (r.ours[k + j] ^ img[k + j]) & m == 0 for j, m in fields)
            want = decode(typ, img, k) if img else None
            r.relocs.append((k, typ, sym, ok, want))
            if sym in unresolved: r.problems.append('UNRESOLVED %s (the image has 0x%X at +%d)' % (sym, want or 0, k))
        results.append(r)
    return results


def side_by_side(r, context=6, maxlines=24):
    lines = []
    if not r.diffs:
        return lines
    first = r.diffs[0]
    ours = d319.disasm(r.ours, r.run)
    img = d319.disasm(d319.words(r.addr, r.size), r.run)
    lo = r.run + max(0, first - context * 2)
    a = [x for x in ours if x[0] >= lo][:maxlines]
    b = [x for x in img if x[0] >= lo][:maxlines]
    fmt = lambda x: ('%06x %-10s %s' % (x[0], d319.fmt_words(x[1]), x[2]))[:52] if x else ''
    lines.append('      %-52s | %s' % ('ours', 'image'))
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else None
        y = b[i] if i < len(b) else None
        mark = '  ' if x and y and x[1] == y[1] and x[0] == y[0] else '**'
        lines.append('   %s %-52s | %s' % (mark, fmt(x), fmt(y)))
    return lines


def match_source(src, funcs=None, wine=None, cgt=None, keep=None, cflags=None):
    """compile and compare; returns (results, notes, unresolved)"""
    src = os.path.abspath(src)
    stem = os.path.splitext(os.path.basename(src))[0]
    work = keep or os.path.join(d319.BUILD, 'match', stem)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    own = wine is None
    w = wine or d319.Wine().__enter__()
    try:
        obj = d319.compile_c(w, src, work, cflags=cflags, cgt=cgt or cgt_of(src), extra=cflags_of(src))
    finally:
        if own: w.__exit__(None, None, None)
    o = d319.Obj(obj)
    frows, drows = map_rows()
    placements, symdefs, targets, notes, unresolved = plan(o, frows, drows)
    if funcs:
        targets = [t for t in targets if t[0].name in funcs]
    out, log = link(obj, work, placements, symdefs)
    results = compare(o, out, targets, unresolved)
    return results, notes, unresolved, o, obj


def report(results, notes, verbose=False):
    ok = True
    for r in results:
        rel = ', '.join('%s %s' % (cname(s or '?'), 'ok' if g else 'WRONG (image 0x%X)' % (w or 0))
                        for k, t, s, g, w in r.relocs[:6] if s and not s.startswith('$'))
        more = '' if len(r.relocs) <= 6 else ' ...'
        if r.exact and all(g for _, _, _, g, _ in r.relocs):
            print('  %-28s %06X %5d  EXACT%s' % (r.name, r.addr, r.size,
                                                  ('  (relocations: %s%s)' % (rel, more)) if verbose and rel else ''))
            continue
        ok = False
        what = []
        if r.diffs: what.append('%d words differ, first at +%d (%06X)' % (len(r.diffs), r.diffs[0], r.addr + r.diffs[0]))
        what += r.problems
        bad = [(s, w) for k, t, s, g, w in r.relocs if not g]
        if bad and not r.plain_diffs: what.append('relocations not as the image: ' + ', '.join('%s (image 0x%X)' % (cname(s or '?'), w or 0) for s, w in bad[:6]))
        print('  %-28s %06X %5d  DIFF: %s' % (r.name, r.addr, r.size, '; '.join(what)))
        for line in side_by_side(r): print(line)
    for n in notes: print('  note:', n)
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src', help='a path, or the name or stem of a file under src/ (e.g. grid_support)')
    ap.add_argument('funcs', nargs='*')
    ap.add_argument('--cgt', help='another compiler home (e.g. .../15.12.3.LTS/ti-cgt-c2000_15.12.3.LTS)')
    ap.add_argument('--keep', help='work directory to keep (default build/match/STEM)')
    ap.add_argument('-v', action='store_true', help='list each relocation of an exact function')
    ap.add_argument('--all-cgt', action='store_true', help='try every installed compiler (toolchain.toml [compiler] others)')
    a = ap.parse_args()
    if not os.path.exists(a.src): a.src = d319.source(a.src)
    if a.all_cgt:
        import glob
        homes = sorted(glob.glob(os.path.expanduser(d319.cfg()['compiler']['others'])))
        with d319.Wine() as w:
            for home in homes:
                home = os.path.dirname(os.path.dirname(home))
                try:
                    results, notes, unresolved, o, obj = match_source(a.src, a.funcs or None, wine=w, cgt=home)
                except d319.ToolError:
                    print('%-28s compile failed' % os.path.basename(home)); continue
                n = sum(1 for r in results if r.exact)
                print('%-28s %d of %d EXACT  %s' % (os.path.basename(home), n, len(results),
                      ' '.join(r.name for r in results if not r.exact)))
        sys.exit(0)
    try:
        results, notes, unresolved, o, obj = match_source(a.src, a.funcs or None, cgt=a.cgt, keep=a.keep)
    except d319.ToolError as e:
        print(e); sys.exit(2)
    print('%s: %s' % (os.path.relpath(a.src), ' '.join(d319.cfg()['compiler']['cflags'] + cflags_of(a.src))))
    if not results:
        for n in notes: print('  note:', n)
        print('  nothing in this file has a map address'); sys.exit(1)
    ok = report(results, notes, a.v)
    n = sum(1 for r in results if r.exact)
    print('%d of %d EXACT' % (n, len(results)))
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
