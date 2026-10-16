"""The D316 gate: prove every source, then link the whole image with TI's linker and compare it
with the original, word for word.

    python3 tools/gate.py [--with SRC ...] [--native] [-v]

1. Verify. Every C file under src/ and its subdirectories (not src/include) is compiled, and
   every .asm file (TI's assembly) assembled by the same cl2000, and compared by match.py: each
   function, switch table, const object and named section (codestart, csmpasswds, csm_rsvd) it
   defines must be EXACT at its map address. Objects are named by their file's stem, so no two
   sources may share a stem (d316.sources checks).
2. Library. tools/libfind.py places the runtime library members (rts2800_fpu32.lib of the
   configured compiler) in the image; their .ebss and constant sections go where the image's
   relocated words say.
3. Link. One lnk2000 run (15.12's own under Wine, or the native one with --native) places:
   - every proved section at its address (a ramfunc with load and run addresses);
   - each library member's sections at theirs;
   - everything else that is not 0xFFFF fill as blobs of the original words (build/gate/blobs.asm,
     one section per run of words), counted as original words;
   - the .cinit records: the output section .cinit is the sources' and the library members' .cinit
     sections in the order toolchain.toml's [gate] cinit_order gives (the original link order of
     the files that have initialised data); the linker ends the table (--rom_model);
   - anything without an address in a parking area outside the device.
   Symbols the objects use and do not define are defined from the map.
4. Compare. hex2000 writes the linked image (0x3E8000..0x3F7FFF, holes 0xFFFF) as a binary like
   the image ([image] bin in toolchain.toml); it must be identical. A report says how many words
   each kind of source supplied.

--with SRC compiles SRC in place of the src/ file with the same stem (to try a change without
touching src/). Exit status 0 when every source is proved and the image is IDENTICAL."""
import argparse, collections, os, re, shutil, struct, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d316, libfind, match

GATE = os.path.join(d316.BUILD, 'gate')
PARK = 0x200000
RAM_PAGE = 1


def sources(extra=()):
    out = d316.sources()            # src/ and its subdirectories, not src/include; stems unique
    stem = lambda p: os.path.splitext(os.path.basename(p))[0]
    for x in extra:
        out = [p for p in out if stem(p) != stem(x)] + [os.path.abspath(x)]
    return out


def cinit_records_of(ws):
    """records of a .cinit section's words: [(var address field offset, count, words)]"""
    out, p = [], 0
    while p < len(ws):
        c = ws[p]
        if c == 0: break
        if c & 0x8000: n, h = 0x10000 - c, 3
        else: n, h = c, 2
        out.append((p, n, h, ws[p:p + h + n]))
        p += h + n
    return out


def image_cinit():
    """the image's .cinit records, in order, as tuples of words"""
    lo, hi = 0x3F336F, 0x3F3B45
    return [tuple(r[3]) for r in cinit_records_of(d316.words(lo, hi - lo))]


def quote(s):
    return '"%s"' % s if re.search(r'[^\w.$]', s) else s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--with', dest='with_', action='append', default=[])
    ap.add_argument('--native', action='store_true', help="link with the native lnk2000, not 15.12's under Wine")
    ap.add_argument('-v', action='store_true')
    a = ap.parse_args()
    t0 = time.time()
    shutil.rmtree(GATE, ignore_errors=True)
    os.makedirs(GATE)
    ok = True
    frows = d316.functions()
    owner = {}
    for r in frows + [x for x in d316.data() if d316.BASE <= x['addr'] < d316.END]:
        for k in range(r['addr'], r['addr'] + r['size']): owner[k] = r

    # ---- 1. verify ---------------------------------------------------------------------------
    proved = {}                     # image address -> source stem
    sections = []                   # (object, section, load, run, page)
    objs = []
    symdefs = {}
    defined_by_src = set()
    cin_img = image_cinit()
    cin_n = 0
    cinit_in = {}                   # source stem -> [(object, .cinit section, size)]
    status_of = {}
    srcs = sources(a.with_)
    print('verify: %d source%s' % (len(srcs), '' if len(srcs) == 1 else 's'))
    with d316.Wine() as w:
        for src in srcs:
            stem = os.path.splitext(os.path.basename(src))[0]
            work = os.path.join(GATE, 'src', stem)
            try:
                results, notes, unresolved, o, obj = match.match_source(src, wine=w, keep=work)
            except d316.ToolError as e:
                print('  %s: FAIL\n%s' % (os.path.relpath(src, d316.ROOT), e)); ok = False; continue
            objs.append(obj)
            fr, dr = match.map_rows()
            placements, sd, targets, pnotes, unres = match.plan(o, fr, dr)
            bad = [r for r in results if not (r.exact and all(g for _, _, _, g, _ in r.relocs))]
            good = [r for r in results if r not in bad]
            parked = [n for n in pnotes if 'not compared' in n or 'no sw_' in n or 'wants' in n]
            status = 'EXACT' if not bad and not parked and not unres else 'FAIL'
            status_of[stem] = status
            nf = sum(1 for r in results if r.kind in ('c', 'asm', 'ramfunc', 'rts'))
            print('  %-28s %-5s %d function%s, %d data object%s' % (os.path.relpath(src, d316.ROOT), status,
                  nf, '' if nf == 1 else 's', len(results) - nf, '' if len(results) - nf == 1 else 's'))
            if status != 'EXACT':
                ok = False
                match.report(results, pnotes)
            for r in results:
                for k in range(r.addr, r.addr + r.size):
                    proved[k] = stem if r in good else None
            for name, base, page in placements:
                sec = o.sections[name]
                run = base
                load = base
                # a function that runs from RAM: load where the map's row is
                for r, sname, off, ld in targets:
                    if sname == name and ld != r.run:
                        load = ld - off; break
                if name.startswith('.cinit'):
                    cinit_in.setdefault(stem, []).append((obj, name, sec.size))
                    cin_n += len(cinit_records_of(sec.data))
                    continue
                if match.PARK <= base < 0x3D0000:     # parked by match.plan
                    load = run = None
                sections.append((obj, name, load, run, page, sec.size))
            defined_by_src.update(o.defined())
            for n, v in sd.items(): symdefs[n] = v
    defs = {n for n in symdefs if n in defined_by_src}
    for n in defs: del symdefs[n]

    # ---- 2. library ----------------------------------------------------------------------------
    lib, found = libfind.find()
    libdir = os.path.dirname(lib)
    rtsdir = libfind.extract(lib)[1]
    lib_words = set()
    lib_secs = []
    member_objs = {}
    for f in found:
        o = member_objs.setdefault(f['member'], d316.Obj(os.path.join(rtsdir, f['member'])))
        lib_secs.append((f['member'], f['section'], f['addr'], 0))
        lib_words.update(range(f['addr'], f['addr'] + f['size']))
    # their data sections and the symbols they use, from the image
    rts_from_image = {}
    placed_lib = {(m, s) for m, s, _, _ in lib_secs}
    for f in found:
        if f['kind'] != 'text': continue
        o = member_objs[f['member']]
        for sym, vals in d316.derive_bases(o.sections[f['section']], f['addr']).items():
            v = collections.Counter(vals).most_common(1)[0][0]
            if sym in o.sections:
                if (f['member'], sym) not in placed_lib:
                    lib_secs.append((f['member'], sym, v, RAM_PAGE if o.sections[sym].kind == 'bss' else 0))
                    placed_lib.add((f['member'], sym))
            elif sym == '(internal)':
                continue
            else:
                defined_here = any(sym in member_objs[m].defined() for m, _, _, _ in lib_secs)
                if not defined_here and sym not in defined_by_src:
                    mv = d316.resolve(sym)
                    if mv is not None and mv != v:
                        print('  library: %s is 0x%X in the image, 0x%X in the map' % (sym, v, mv)); ok = False
                    rts_from_image[sym] = v
    # members named in the .cinit order: their .cinit goes into .cinit; a member no code of the
    # image contains (errno.obj: only data) is linked for it, its .ebss where the image's code
    # has the symbol
    order = d316.cfg()['gate'].get('cinit_order', [])
    for e in order:
        if not e.startswith('rts:'): continue
        m = e[4:]
        if m not in member_objs:
            o = member_objs[m] = d316.Obj(os.path.join(rtsdir, m))
            for n, (sn, v) in o.defined().items():
                if o.sections[sn].kind == 'bss' and n in rts_from_image and (m, sn) not in placed_lib:
                    lib_secs.append((m, sn, rts_from_image[n] - v, RAM_PAGE)); placed_lib.add((m, sn))
        for name in member_objs[m].sections:
            if name.startswith('.cinit'): placed_lib.add((m, name))
    # the members' other sections (.cinit, data no code uses) are parked
    for m, o in member_objs.items():
        for name, sec in o.sections.items():
            if sec.kind in ('text', 'data', 'bss') and sec.size and not name.startswith('.debug') and (m, name) not in placed_lib:
                lib_secs.append((m, name, None, 0))
    # the linker makes these itself (--rom_model): the start of .cinit, no copy table, no .pinit,
    # the start of .stack; the compare checks them
    LINKER_SYMS = ('cinit', 'binit', 'pinit', '__stack', '___TI_pprof_out_hndl')     # (the last: -1, no profiling)
    for n, v in rts_from_image.items():
        if n not in LINKER_SYMS: symdefs.setdefault(n, v)
    # a symbol a placed member defines (c_int00, which start calls; FS$$DIV for a float division)
    # comes from the member, not
    # from the map: defining it here would keep the linker from taking the member
    for m, o in member_objs.items():
        for n in o.defined(): symdefs.pop(n, None)

    # ---- 3. blobs and the link -----------------------------------------------------------------
    supplied = {}
    for k in range(d316.BASE, d316.END):
        if k in proved: supplied[k] = 'proved' if proved[k] else 'unproved'
        elif k in (0x3F3B43, 0x3F3B44): supplied[k] = 'linker'      # the end of .cinit (--rom_model)
        elif k in lib_words: supplied[k] = 'library'
        elif k not in owner and d316.W(k) == 0xFFFF: supplied[k] = 'fill'
        else: supplied[k] = 'original'
    # the output section .cinit, whose start the linker gives c_int00 as cinit (--rom_model), and
    # after which it writes the 0 that ends the table: the input sections in the original order
    CINIT_LO, CINIT_HI = 0x3F336F, 0x3F3B43
    cinit_spec, k = [], CINIT_LO
    for e in order:
        if e.startswith('rts:'):
            m = e[4:]
            # (.cinit) takes the member's subsections too (_lock.obj's .cinit:__lock, .cinit:__unlock),
            # in the order the linker gives them
            n = sum(sec.size for name, sec in member_objs[m].sections.items() if name.startswith('.cinit'))
            cinit_spec.append('rts2800_fpu32.lib<%s>(.cinit)' % m)
            for j in range(k, k + n): supplied[j] = 'library'
            k += n
        elif e in cinit_in:
            for obj, name, size in cinit_in.pop(e):
                cinit_spec.append('"%s"(%s)' % (obj, name))
                for j in range(k, k + size): supplied[j] = 'proved' if status_of.get(e) == 'EXACT' else 'unproved'
                k += size
        else:
            print('  cinit_order: %s has no .cinit' % e); ok = False
    for stem in cinit_in:
        print('  %s: .cinit records, but not in [gate] cinit_order' % stem); ok = False
    if k != CINIT_HI:
        print('  cinit: the sections in cinit_order are %d words, the image\'s records %d' % (k - CINIT_LO, CINIT_HI - CINIT_LO))
        ok = False
    # runs of original words, at most 16K words
    breaks = {CINIT_LO, CINIT_HI}
    blobs = []
    k = d316.BASE
    while k < d316.END:
        if supplied[k] != 'original': k += 1; continue
        j = k + 1
        while j < d316.END and supplied[j] == 'original' and j - k < 0x4000 and j not in breaks: j += 1
        blobs.append((k, j))
        k = j
    with open(os.path.join(GATE, 'blobs.asm'), 'w') as f:
        f.write('; the words of the image that no proved source or library member supplies yet\n')
        for lo, hi in blobs:
            f.write('\t.sect "blob_%06X"\n' % lo)
            ws = d316.words(lo, hi - lo)
            for i in range(0, len(ws), 16):
                f.write('\t.word ' + ','.join('0x%04X' % x for x in ws[i:i + 16]) + '\n')
    d316.run([d316.native('cl2000')] + d316.cfg()['compiler']['cflags'][:4] + ['--abi=coffabi', '-c', 'blobs.asm'], cwd=GATE)
    objs.append(os.path.join(GATE, 'blobs.obj'))

    cmd = os.path.join(GATE, 'gate.cmd')
    park = PARK
    with open(cmd, 'w') as f:
        f.write('-a\n-o gate.out\n-m gate.map\n--rom_model\n--stack_size=0x400\n--disable_auto_rts\n--disable_clink\n')
        f.write('-i "%s"\n-l rts2800_fpu32.lib\n' % libdir)
        # a member is only taken from the library for a symbol nobody defines: ask for one of each
        for m, o in sorted(member_objs.items()):
            g = sorted(n for n, (sn, v) in o.defined().items() if o.sections[sn].kind == 'text') or sorted(o.defined())
            if g: f.write('-u %s\n' % g[0])
        for o in objs: f.write('"%s"\n' % o)
        # the whole address space in ranges, so that sections at fixed addresses are in the map,
        # and parking ranges outside the device for what is not compared
        f.write('MEMORY\n{\n')
        f.write('  PAGE 0: LOW0 : origin = 0x0, length = 0x%X\n' % PARK)
        f.write('  PAGE 0: PARKP : origin = 0x%X, length = 0x80000\n' % PARK)
        f.write('  PAGE 0: HIGH0 : origin = 0x%X, length = 0x%X\n' % (PARK + 0x80000, 0x3FFFC0 - PARK - 0x80000))
        f.write('  PAGE 1: LOW1 : origin = 0x0, length = 0x%X\n' % (PARK + 0x80000))
        f.write('  PAGE 1: PARKD : origin = 0x%X, length = 0x80000\n' % (PARK + 0x80000))
        f.write('  PAGE 1: HIGH1 : origin = 0x%X, length = 0x%X\n}\n' % (PARK + 0x100000, 0x3FFFC0 - PARK - 0x100000))
        f.write('SECTIONS\n{\n')
        for lo, hi in blobs:
            f.write('  blob_%06X : load = 0x%X, PAGE = 0\n' % (lo, lo))
        f.write('  .cinit : {\n%s  } load = 0x%X, PAGE = 0\n' % (''.join('    %s\n' % s for s in cinit_spec), CINIT_LO))
        f.write('  .stack : load = 0xE390, PAGE = 1\n')
        for obj, name, load, run, page, size in sections:
            spec = '{ "%s"(%s) }' % (obj, name)
            out = '%s_%s' % (os.path.splitext(os.path.basename(obj))[0], re.sub(r'\W', '_', name))
            if load is None:
                f.write('  %s : %s > %s\n' % (out, spec, 'PARKP' if page == 0 else 'PARKD, PAGE = 1'))
            elif load != run:
                f.write('  %s : %s load = 0x%X, run = 0x%X, PAGE = %d\n' % (out, spec, load, run, page))
            else:
                f.write('  %s : %s load = 0x%X, PAGE = %d\n' % (out, spec, load, page))
        for m, name, addr, page in lib_secs:
            out = 'rts_%s_%s' % (m[:-4], re.sub(r'\W', '_', name))
            spec = '{ rts2800_fpu32.lib<%s>(%s) }' % (m, name)
            if addr is None:
                f.write('  %s : %s > PARKD, PAGE = 1\n' % (out, spec))
            else:
                f.write('  %s : %s load = 0x%X, PAGE = %d\n' % (out, spec, addr, page))
        f.write('}\n')
        for n, v in sorted(symdefs.items()):
            f.write('%s = 0x%X;\n' % (n, v))
    linker = 'native' if a.native else d316.cfg()['gate'].get('linker', 'cgt')
    if linker == 'native':
        p = d316.run([d316.native('lnk2000'), 'gate.cmd'], cwd=GATE, check=False)
    else:
        with d316.Wine() as w:
            p = w.run([d316.cgt_bin('lnk2000'), 'gate.cmd'], cwd=GATE, check=False)
    out = os.path.join(GATE, 'gate.out')
    log = (p.stdout + p.stderr).strip()
    if p.returncode or not os.path.exists(out):
        print('link: FAIL\n' + log); sys.exit(1)
    if log and a.v: print(log)

    # the linked .cinit records, in order, must be the image's
    lo = d316.Obj(out)
    got = [tuple(r[3]) for r in cinit_records_of(lo.sections['.cinit'].data)] if '.cinit' in lo.sections else []
    cin_bad = [i for i in range(max(len(got), len(cin_img))) if i >= len(got) or i >= len(cin_img) or got[i] != cin_img[i]]

    # ---- 4. compare --------------------------------------------------------------------------
    hexcmd = os.path.join(GATE, 'hex.cmd')
    with open(hexcmd, 'w') as f:
        # hex2000's ROMS addresses count bytes here: twice the word address
        f.write('gate.out\n-b\n-image\n-order MS\n-romwidth 16\n-fill 0xFFFF\n')
        f.write('ROMS\n{\n  FLASH: origin = 0x%X, length = 0x%X, romwidth = 16, files = { gate.bin }\n}\n'
                % (2 * d316.BASE, 2 * (d316.END - d316.BASE)))
    d316.run([d316.native('hex2000'), 'hex.cmd'], cwd=GATE)
    built = open(os.path.join(GATE, 'gate.bin'), 'rb').read()
    orig = open(os.path.join(d316.ROOT, d316.cfg()['image']['bin']), 'rb').read()
    bw = struct.unpack('>%dH' % (len(built) // 2), built)
    diffs = [d316.BASE + i for i in range(min(len(bw), len(d316.image()))) if bw[i] != d316.image()[i]]
    identical = built == orig

    count = collections.Counter(supplied.values())
    print('library: %d sections of %d members of %s placed' % (len(found), len(member_objs), os.path.relpath(lib, d316.cfg()['compiler']['cgt'])))
    if rts_from_image and a.v:
        print('  from the image:', ', '.join('%s=0x%X' % kv for kv in sorted(rts_from_image.items())))
    print('cinit: %d record%s linked (%d from the sources, %d from the library), %s' % (
          len(got), '' if len(got) == 1 else 's', cin_n, len(got) - cin_n,
          "the image's records in the image's order" if not cin_bad else
          '%d NOT as the image, the first at record #%d' % (len(cin_bad), cin_bad[0])))
    if cin_bad: ok = False
    ver = re.search(r'(\d+\.\d+\.\d+)', os.path.basename(d316.cfg()['compiler']['cgt'])).group(1)
    print('link: lnk2000 %s, %d blob sections' % ('18.12.8 (native)' if linker == 'native' else ver + ' (Wine)', len(blobs)))
    print()
    print('| Supplied by | Words | Share |')
    print('|---|---|---|')
    for k, label in (('proved', 'Proved sources (src/)'), ('library', 'Runtime library members'),
                     ('linker', 'Made by the linker (end of .cinit)'),
                     ('fill', 'Fill 0xFFFF (holes)'), ('original', 'Original words (blobs)'),
                     ('unproved', 'Sources that do not match (linked as compiled)')):
        if k == 'unproved' and not count[k]: continue
        print('| %s | %d | %.1f%% |' % (label, count[k], 100.0 * count[k] / 65536))
    print()
    if identical:
        print('IDENTICAL: the linked image is %s word for word' % os.path.relpath(os.path.join(d316.ROOT, d316.cfg()['image']['bin']), d316.ROOT))
    else:
        ok = False
        print('DIFFERENT: %d words differ%s' % (len(diffs), '' if len(bw) == 65536 else ' (size %d words)' % len(bw)))
        for d in diffs[:12]:
            r = owner.get(d)
            print('  %06X  built %04X  image %04X  %s (%s)' % (d, bw[d - d316.BASE], d316.W(d),
                  r['name'] if r else '-', supplied.get(d)))
    print('%s  (%.1f s)' % ('PASS' if ok else 'FAIL', time.time() - t0))
    d316.stop_wine()
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
