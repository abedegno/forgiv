"""The modding build: compile every source and link a working ARMStore.bin in one armlink run,
with the application's code and constants free to move and change size. It needs a licensed MDK
(the image is 145 KB, over MDK-Lite's 32 KB link limit): MDK_LICENSED_HOME or mdk_licensed_home
in toolchain.local.toml; unset, it says so and stops (exit status 0). It does not prove the
sources; tools/gate.py stays the proof that they are the original.

    $EXHUME/.venv/bin/python tools/modbuild.py [--src DIR | --patch FILE ...] [--shift N]
        [--version N] [--out PATH]

--src DIR     build the sources in DIR (a copy of src/, same layout) in place of src/
--patch FILE  copy src/ to build/mod/tree/src, apply FILE there with `patch -p1` (paths in the
              patch relative to the a316 directory: a/src/..., b/src/...) and build that; may be
              given more than once. src/ itself is never changed.
--shift N     start the floating code N bytes later (a multiple of 4): an audit that everything
              that floats follows a move
--version N   the version in the file's header (default [gate] version, 316)
--out PATH    the package to write (default build/mod/ARMStore.bin); the HEX, the linked ELF and
              armlink's listing go beside it (ARMStore.hex, .axf, .map)

What is fixed and what floats:

  fixed   0x08014000      the vector table (the startup file's RESET): the boot loader below
                          0x08014000 starts the application from it, and nvic_init points VTOR
                          at it (NVIC_SetVectorTable(0x8000000, 0x14000))
          0x08013FFC      the version word the update compares with is the boot loader's, below
                          the application: outside this build, which writes the version in the
                          file's header
          RAM             the RW and ZI layout of the original: the image's own initial RAM
                          around the proved sources' RAM sections (stm32f10x_rcc's .data, the
                          startup file's HEAP and STACK), the library's __libspace, one RW region
                          and one ZI region in the region table. The sources reach almost every
                          variable through the map's names, defined at fixed addresses
  floats  0x08014150..0x0803FFFF (the rest of the 256 KB flash): the library's start-up code,
          every .text, the library members, the .constdata, the region table and the compressed
          RW data, laid out by armlink in its own order (attribute, name, then input order, the
          objects given in the original's order). New code goes in with the rest. New variables
          go in RW_IRAM2, from the end of the original's stack (0x2000A598) to the end of the
          64 KB RAM; the region table then has entries for it too.

Names for numbers. The build always works on a copy of the sources (build/mod/tree). In it, a
function's Thumb address written as a number in C (main's OSTaskCreate(0x802735b, ...) for
task_measure, USBH_USR_EnumerationDone's 0x8022c51 for modbus_slave_process: 9 in all) becomes
the function's name. The number compiles to the same literal word with no relocation, so it would
not follow its function when the code moves; the name gives the same bytes with a relocation.
src/ keeps the numbers (they are what the gate proves).

Checks (any failure gives exit status 1, and no package is written):
  1. link: no unresolved symbol; no name of a flash address given as a number (an alias is
     resolved to the name of what is there, with a steering file); every function of
     map/functions.tsv in the image.
  2. fixed: the vector table at 0x08014000 with the original's initial SP, every RAM blob and
     proved RAM section at its address, the region table's RW and ZI entries as the original's,
     the image inside the application flash.
  3. objects: each section of each object compared with the gate's object of the same source
     (build/gate/verify, from the last tools/gate.py run): identical or listed as changed, with
     the words that changed.
  4. moved, not changed: every section the mod did not change, compared with the original image
     where the original has it; every relocated field must hold the original's target moved by as
     much as the section holding the target moved, and every other byte must be the same. The
     library members are checked the same way, with their relocations read from the library.
  5. numbers: in every section of the build's objects and of the library members, every word in
     the literal pools and constant data (4-byte aligned, in a section armcc aligned for words)
     that holds an address in the floating flash, and every MOVW/MOVT pair that makes one, is a
     relocated field (a number would not follow a move). The gate's objects are audited the same
     way and their numbers listed: the ones the names-for-numbers step replaces.

tools/emudiff.py then runs functions of the build and of the original side by side in an
emulator (Exhume's differential tester).

Built from an unchanged tree, the image comes out as the original, byte for byte. --shift shows
that nothing depends on that.

src/clib holds C stand-ins for two library members (heapauxi.o, use_no_semi.o); the build leaves
them out and links the members from the library, as the original did."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import argparse, bisect, concurrent.futures, pickle, re, shutil, struct, subprocess, time  # noqa: E402
import gate as G  # noqa: E402  (a316env, Exhume's tools, the gate's layout helpers)
import a316env, objplace  # noqa: E402
from gate import armtc, armmap, ehex, rwdata, stem, rel  # noqa: E402

ROOT = G.ROOT
MOD = os.path.join(ROOT, 'build', 'mod')
GATE_DIR = os.path.join(ROOT, 'build', 'gate')
FLASH_END = 0x08040000            # STM32F105/107 connectivity line, 256 KB (xC): docs/development/modding.md
RAM_END = 0x20010000              # 64 KB of SRAM on the same parts
FLOAT_LO = 0x08014150             # after the vector table


def tree_sources(srcdir):
    out, inc = [], os.path.join(srcdir, 'include')
    for d, dirs, files in os.walk(srcdir):
        if os.path.commonpath([d, inc]) == inc: dirs[:] = []; continue
        dirs.sort()
        out += [os.path.join(d, n) for n in sorted(files) if n.endswith(('.c', '.s'))]
    seen = {}
    for p in out:
        if stem(p) in seen: sys.exit(f'two sources with the stem {stem(p)}')
        seen[stem(p)] = p
    return out


def compile_one(tc, cfg, src, work):
    os.makedirs(work, exist_ok=True)
    obj = os.path.join(work, stem(src) + '.o')
    text = open(src, encoding='latin1').read()
    if src.endswith('.s'):
        m = re.search(r'^\s*;\s*opts:\s*(.*)$', text, re.M)
        ok, log = tc.assemble(src, obj, m.group(1) if m else (cfg.asm_opts or '--cpu Cortex-M3'))
    else:
        _, opts = cfg.directives(src)
        ok, log = tc.compile(src, obj, opts or cfg.c_opts)
    return obj if ok else None, log


def gate_state(cfg, m):
    """The last gate run: {stem: Proved} with each source's placements in the original, the
    layout, and the library link's listing."""
    cdir = os.path.join(GATE_DIR, 'cache', 'src')
    lib_lst = os.path.join(GATE_DIR, 'library', 'link.lst')
    if not os.path.isdir(cdir) or not os.path.exists(lib_lst):
        sys.exit('modbuild: no gate run in build/gate (run tools/gate.py first: the build compares with its objects)')
    proved = {}
    for s in G.sources(cfg):
        cp = os.path.join(cdir, stem(s) + '.pkl')
        if not os.path.exists(cp): sys.exit(f'modbuild: {rel(s)} is not in the last gate run (run tools/gate.py)')
        c = pickle.load(open(cp, 'rb'))
        if not c['whole'] or not c['obj'] or not os.path.exists(c['obj']):
            sys.exit(f'modbuild: {rel(s)} did not pass the last gate run')
        proved[s] = G.Proved(s, c['obj'], c['info'], c['whole'], c['lines'], c['key'], cached=True, pobj=c.get('pobj'))
    img = ehex.Image(cfg.image)
    lay = G.Layout(img.base, img.end, rwdata._table_rt2(img), img.end, [], [])
    secs, problems = G.place_all(cfg, m, proved, print)
    if problems: sys.exit('modbuild: the gate state does not place: ' + '; '.join(problems))
    lay.sections = secs
    lay.flash = [x for x in secs if lay.base <= x[3] < lay.end]
    return proved, lay, img, open(lib_lst, encoding='latin1').read()


class Info:
    """resolve_refs' view of an objplace.Obj."""
    def __init__(self, o): self.symbols = list(o.symbols.values())


def alloc(o):
    return {s.name: (i, s) for i, s in o.sections.items() if 'SHF_ALLOC' in s.flags and s.size}


def relocs_by_name(o):
    return {o.sections[i].name: sorted(rs, key=lambda r: r.offset) for i, rs in o.relocs.items() if i in o.sections}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    g = ap.add_mutually_exclusive_group()
    g.add_argument('--src', help='a copy of src/ to build')
    g.add_argument('--patch', action='append', default=[], help='a patch to apply to a copy of src/')
    ap.add_argument('--shift', type=lambda s: int(s, 0), default=0)
    ap.add_argument('--version', type=int)
    ap.add_argument('--out', default=os.path.join(MOD, 'ARMStore.bin'))
    a = ap.parse_args()
    t0 = time.time()
    home = a316env.require_licensed('modbuild')
    if a.shift % 4 or a.shift < 0: sys.exit('modbuild: --shift must be a non-negative multiple of 4')
    cfg = G.load_cfg()
    gc = G.gate_cfg(cfg)
    version = a.version if a.version is not None else gc['version']
    m = armmap.Map.from_dir(cfg.map)
    errors = []

    # ---- the tree -----------------------------------------------------------------------------
    os.makedirs(MOD, exist_ok=True)
    tree = os.path.join(MOD, 'tree')
    shutil.rmtree(tree, ignore_errors=True)
    shutil.copytree(os.path.abspath(a.src) if a.src else os.path.join(ROOT, 'src'), os.path.join(tree, 'src'))
    srcdir = os.path.join(tree, 'src')
    os.symlink(os.path.join(ROOT, 'lib'), os.path.join(tree, 'lib'))     # the StdPeriph files include ../../lib/stdperiph35
    if a.patch:
        for p in a.patch:
            r = subprocess.run(['patch', '-p1', '--no-backup-if-mismatch', '-i', os.path.abspath(p)],
                               cwd=tree, capture_output=True, text=True)
            print(f'patch {os.path.relpath(p)}:\n  ' + (r.stdout + r.stderr).strip().replace('\n', '\n  '))
            if r.returncode: print('FAIL: the patch does not apply'); sys.exit(1)
    proved, lay, img, lib_listing = gate_state(cfg, m)
    skip = G.stand_ins(lay, gc)
    srcs = [s for s in tree_sources(srcdir) if stem(s) not in skip]
    gate_by_stem = {stem(p.src): p for p in proved.values()}
    print(f'modbuild: {len(srcs)} sources from {os.path.relpath(a.src, ROOT) if a.src else "src"}, copied to build/mod/tree (left out: the library stand-ins '
          f'{", ".join(sorted(skip))}); version {version}; floating code shifted by 0x{a.shift:X}')
    for p in (a.out,):
        if os.path.exists(p): os.remove(p)
    named = names_for_numbers(srcs, m, gc)
    print(f'names for numbers: {sum(len(v) for v in named.values())} function addresses written as numbers in the sources, '
          f'given their names in the copy: ' +
          ('; '.join(f'{os.path.basename(f)}: ' + ', '.join(f'{x} -> {n}' for x, n in v) for f, v in sorted(named.items())) or 'none'))

    # ---- compile (the gate's compiler: the compile home, MDK-Lite is enough) -----------------------
    a316env.start_licensed()
    os.environ['WINEPREFIX'] = a316env.LICENSED_PREFIX           # every Keil tool here runs in that prefix
    try:
        tc = armtc.Toolchain(cfg.home, ROOT)
        ltc = armtc.Toolchain(home, ROOT)
        objdir = os.path.join(MOD, 'obj')
        shutil.rmtree(objdir, ignore_errors=True)
        with concurrent.futures.ThreadPoolExecutor(4) as ex:
            res = list(ex.map(lambda s: (s,) + compile_one(tc, cfg, s, os.path.join(objdir, stem(s))), srcs))
        objpath = {}
        for s, obj, log in res:
            if obj is None:
                print(f'compile: FAIL {rel(s)}\n  ' + log.strip().replace('\n', '\n  ')); sys.exit(1)
            objpath[stem(s)] = obj
        print(f'compile: {len(objpath)} objects ({os.path.basename(cfg.home)}, the gate\'s compiler and options)')
        with concurrent.futures.ThreadPoolExecutor(4) as ex:
            objs = dict(zip(objpath, ex.map(lambda st: objplace.read_object(tc, objpath[st]), objpath)))
            gobjs = dict(zip(gate_by_stem, ex.map(lambda st: objplace.read_object(tc, gate_by_stem[st].obj), gate_by_stem)))

        # the library members the gate's library link loaded, read with their relocations
        libmem = library_members(tc, gc, lib_listing)

        # ---- the link ---------------------------------------------------------------------------
        order = G.link_order(lay, [st for st in objpath if st in gate_by_stem]) + sorted(st for st in objpath if st not in gate_by_stem)
        startup = next((st for st in order if 'RESET' in alloc(objs[st])), None)
        if startup is None: sys.exit('modbuild: no source has a RESET section (the vector table)')
        extra_names = {}
        for s in srcs:
            for n, ad in armmap.annotations(open(s, encoding='latin1').read()).items():
                if n not in m.by_name: extra_names[n] = ad
        entries, steer, serr = G.resolve_refs(m, [Info(objs[st]) for st in order], extra_names, gc, lay.base, FLASH_END)
        errors += serr
        ram_secs = sorted((ad, z, st, os.path.join(objdir, st, st + '.o'), sec) for st, o, sec, ad, z in lay.sections
                          if ad >= 0x20000000 and st in objpath)
        files, lead, tail, rsel, rchecks, rerr, rw, zi = G.ram_parts(img, gc, ram_secs)
        errors += rerr
        zi_end = zi.exec + zi.size
        work = os.path.join(MOD, 'link')
        shutil.rmtree(work, ignore_errors=True); os.makedirs(work)
        files['mod.sym'] = armmap.symdefs_text(entries)
        files['mod.steer'] = '\n'.join(steer) + '\n'
        files['link.sct'] = f"""; the A316 modding build (tools/modbuild.py)
LR_IROM1 0x{lay.base:08X} 0x{FLASH_END - lay.base:08X} {{      ; the application flash, to the end of the 256 KB
  ER_VECT 0x{lay.base:08X} FIXED {{                ; fixed: the boot loader starts the application here
    {startup}.o (RESET, +First)
  }}
  ER_IROM1 0x{FLOAT_LO + a.shift:08X} FIXED {{             ; floats: code, constants, the region table (FIXED: a root region)
    *(InRoot$$Sections)
    .ANY (+RO)
  }}
  RW_IRAM1 0x{rw.exec:08X} 0x{zi_end - rw.exec:08X} {{   ; fixed: the original's RW and ZI layout
    {chr(10).join('    ' + x for x in rsel).strip()}
  }}
  RW_IRAM2 0x{zi_end:08X} 0x{RAM_END - zi_end:08X} {{   ; new variables, above the original's stack
    .ANY (+RW +ZI)
  }}
}}
"""
        keep = []
        for st in order: keep += ['--keep', f'{st}.o(*)']
        for o in lead + tail: keep += ['--keep', f'{o}(*)']
        job = dict(work=work, files=files, assemble=[n for n in files if n.endswith('.s')], home=home,
                   inputs=[os.path.join(work, o) for o in lead] + [objpath[st] for st in order] +
                          [os.path.join(work, o) for o in tail] + [os.path.join(work, 'mod.sym')],
                   args=keep + ['--entry', 'Reset_Handler', '--edit', os.path.join(work, 'mod.steer'),
                                '--libpath', os.path.join(gc['libhome'], 'ARM', 'ARMCC', 'lib'),
                                '--diag_suppress', '6305,6314,6329,6332'])
        if errors:
            for e in errors: print('  FAIL ' + e)
            print('FAIL'); sys.exit(1)
        G._W['tc'] = ltc
        r = G.task_link(job)
        if not r.get('ok'):
            print('link: FAIL\n  ' + r.get('log', '').strip()[-4000:].replace('\n', '\n  ')); sys.exit(1)
        warns = [l for l in r['log'].splitlines() if 'Warning' in l]
        print(f'link: armlink ({os.path.basename(home.rstrip(os.sep))}, licensed), one run, {len(steer)} aliases resolved '
              f'by name, {len(entries)} RAM and peripheral names from the map' + (f', {len(warns)} warnings' if warns else ''))
        for w in warns: print('  ' + w)
    finally:
        a316env.stop_licensed()

    listing = r['listing']
    new = {}                                    # (section, object) -> [addr]
    for ad, z, sec, ob in G.listing_sections(listing): new.setdefault((sec, ob), []).append((ad, z))
    flash = G.whole_image(r, lay)
    image_end = lay.base + len(flash)
    built = bytes(flash)
    syms = {}
    for l in listing.splitlines():
        mm = G._LSYM.match(l)
        if mm: syms.setdefault(mm.group(1), int(mm.group(2), 16))

    # ---- 1. link: every function of the map ------------------------------------------------------
    missing = [row.name for row in m.functions if not any(x <= row.addr < y for x, y in gc['lib_ranges'])
               and armmap.placeholder(row.name) is None and row.name not in syms]
    nf = sum(1 for row in m.functions if not any(x <= row.addr < y for x, y in gc['lib_ranges']) and armmap.placeholder(row.name) is None)
    print(f'1. link: no unresolved symbol, no flash address given as a number; {nf - len(missing)} of {nf} named functions of '
          f'map/functions.tsv in the image' + (f'; MISSING: {", ".join(missing[:10])}' if missing else ''))
    if missing: errors.append('functions missing')

    # ---- 2. fixed -------------------------------------------------------------------------------
    fx = []
    vec = new.get(('RESET', f'{startup}.o'), [(None, 0)])[0]
    sp = struct.unpack_from('<I', built, 0)[0]
    fx.append((f'vector table ({startup}.o RESET) at 0x{lay.base:08X}, initial SP 0x{sp:08X} as the original\'s', vec[0] == lay.base and sp == struct.unpack_from('<I', img.data, 0)[0]))
    bad_ram = []
    for what, ad, sec, ob in rchecks:
        if ad not in [x for x, _ in new.get((sec, ob), [])]: bad_ram.append(f'{what} not at 0x{ad:08X}')
    ls = [x for x, _ in new.get(('.bss', 'c_w.l(libspace.o)'), [])]
    if gc['libspace'] not in ls: bad_ram.append(f'__libspace at {ls}, not 0x{gc["libspace"]:08X}')
    fx.append(('RAM: ' + (f'every RW blob, proved RAM section (stm32f10x_rcc .data, HEAP, STACK) and __libspace at its original address'
                          if not bad_ram else 'MOVED: ' + '; '.join(bad_ram[:6])), not bad_ram))
    try:
        regions = rwdata.find_table(ehex_image(lay.base, built))
        o_regions = rwdata.find_table(img)
        same = [(x.exec, x.size, x.kind) for x in regions[:len(o_regions)]] == [(x.exec, x.size, x.kind) for x in o_regions]
        extra_r = regions[len(o_regions):]
        fx.append(('region table: ' + ', '.join(f'{x.kind} 0x{x.exec:08X}+0x{x.size:X}' for x in regions) +
                   (' (the original\'s two, then the new variables\')' if extra_r else ' (the original\'s two)'), same))
    except Exception as e:
        fx.append((f'region table: not read ({e})', False))
    fx.append((f'image 0x{lay.base:08X}-0x{image_end:08X} inside the application flash (to 0x{FLASH_END:08X})', image_end <= FLASH_END))
    bad_fx = [t for t, ok in fx if not ok]
    print('2. fixed: ' + ('all as required' if not bad_fx else f'{len(bad_fx)} FAIL'))
    for t, ok in fx: print(f'   {"ok  " if ok else "FAIL"} {t}')
    if bad_fx: errors.append('fixed layout')

    # ---- 3. objects against the gate's -----------------------------------------------------------
    changed, same_n, relonly = {}, 0, {}
    new_files = [st for st in objs if st not in gobjs]
    for st in order:
        if st not in gobjs: continue
        a1, a2 = alloc(objs[st]), alloc(gobjs[st])
        r1, r2 = relocs_by_name(objs[st]), relocs_by_name(gobjs[st])
        for n in sorted(set(a1) | set(a2)):
            s1, s2 = a1.get(n, (None, None))[1], a2.get(n, (None, None))[1]
            rl = lambda rs: [(x.offset, x.tname, x.sname) for x in rs]
            if s1 is not None and s2 is not None and s1.size == s2.size and (s1.data != s2.data or rl(r1.get(n, [])) != rl(r2.get(n, []))):
                # names for numbers: the only differences are words that are new relocated fields
                # (the object holds the addend, 0, where the gate's held the number)
                added = {x for x in rl(r1.get(n, [])) if x not in rl(r2.get(n, []))}
                gone = [x for x in rl(r2.get(n, [])) if x not in rl(r1.get(n, []))]
                fields = set()
                for off, tn, sn in added: fields.update(range(off, off + 4) if tn == 'R_ARM_ABS32' else ())
                if added and not gone and all(s1.data[i] == s2.data[i] or i in fields for i in range(s1.size)):
                    relonly.setdefault(st, []).append((n, len(added))); continue
            if s1 is None or s2 is None or s1.data != s2.data or s1.size != s2.size:
                changed.setdefault(st, []).append(n + ('' if s1 and s2 else ' (new)' if s1 else ' (removed)'))
            elif rl(r1.get(n, [])) != rl(r2.get(n, [])):
                relonly.setdefault(st, []).append((n, len(r1.get(n, [])) - len(r2.get(n, []))))
            else:
                same_n += 1
    print(f'3. objects: {same_n} sections identical to the gate\'s objects, {sum(len(v) for v in relonly.values())} different only in '
          f'words that became relocated fields (names for numbers), {sum(len(v) for v in changed.values())} changed' +
          (f', {len(new_files)} new files' if new_files else ''))
    for st, ns in sorted(relonly.items()):
        print(f'   names for numbers: {st}: ' + ', '.join(f'{n} (+{k} relocations)' for n, k in ns))
    for st, ns in sorted(changed.items()):
        print(f'   changed: {st}: {", ".join(ns)}')
        a1, a2 = alloc(objs[st]), alloc(gobjs[st])
        for n in ns:
            if n not in a1 or n not in a2: continue
            s1, s2 = a1[n][1], a2[n][1]
            at = new.get((n, f'{st}.o'), [(None, 0)])[0][0]
            if s1.size != s2.size:
                print(f'     {n}: {s1.size} bytes, {s2.size} in the original (what follows it moves)'); continue
            for i in range(0, s1.size, 2):
                if s1.data[i:i + 2] != s2.data[i:i + 2]:
                    print(f'     {n}+0x{i:X} at 0x{at + i:08X}: {s1.data[i:i + 2][::-1].hex()}, original {s2.data[i:i + 2][::-1].hex()} (halfwords)')
    for st in new_files: print(f'   new file: {st}')

    # ---- 4. moved, not changed; 5. numbers --------------------------------------------------------
    res = moved_check(lay, img, built, new, objs, gobjs, libmem, lib_listing, changed, order, gc, syms, m)
    orig_numbers = audit_numbers({st: gobjs[st] for st in gobjs if st not in skip})
    print(f'4. moved, not changed: {res["sections"]} sections ({res["bytes"]} bytes) compared with the original where it has them: '
          f'{res["same"]} bytes identical, {res["moved"]} bytes moved; {res["fields"]} relocated fields checked, '
          f'{res["followed"]} of them hold a moved target, each moved by as much as its target'
          + (f', {res["weak"]} a weak reference to nothing (the same bytes)' if res['weak'] else '')
          + (f'; {res["named"]} point into a changed section and follow their target by name' if res['named'] else '')
          + (f'; {len(res["unchecked"])} point inside a changed section by offset (not checked)' if res['unchecked'] else '')
          + ('' if not res['bad'] else f'; {len(res["bad"])} FAIL'))
    # the bytes between sections: armlink's alignment PAD rows, region ends rounded up to 4 and the
    # load region's filler before a FIXED region (--shift); all must be zero
    tb = new.get(('Region$$Table', 'anon$$obj.o'), [(lay.table[0], 0)])[0]
    rows = [(ad, ad + z) for ad, z, sec, ob in G.listing_sections(listing) if lay.base <= ad < FLASH_END]
    pads = [(int(x.group(1), 16), int(x.group(1), 16) + int(x.group(2), 16)) for x in
            re.finditer(r'^\s*0x([0-9a-fA-F]{8})\s+0x([0-9a-fA-F]{8})\s+PAD\s*$', listing, re.M) if lay.base <= int(x.group(1), 16) < FLASH_END]
    between = G.gaps(lay.base, tb[0] + tb[1], rows + pads)
    nz = [(x, y) for x, y in between + pads if any(built[x - lay.base:y - lay.base])]
    print(f'   between the sections: {sum(y - x for x, y in pads)} bytes in {len(pads)} alignment PAD rows and {sum(y - x for x, y in between)} '
          f'bytes of other filler (a region rounded up, the --shift gap), ' +
          ('all zero' if not nz else 'NOT ZERO at ' + ', '.join(f'0x{x:08X}' for x, y in nz[:6])))
    if nz: errors.append('padding')
    for b in res['bad'][:16]: print('   FAIL ' + b)
    if res['absent']: print('   original sections not in the build: ' + ', '.join(res['absent'][:12]))
    if res['bad']: errors.append('moved check')
    print(f'5. numbers: the gate\'s objects (the sources as proved) hold {len(orig_numbers)} floating flash addresses as numbers'
          + (' (' + ', '.join(orig_numbers) + ')' if orig_numbers else '') + '; in the build, '
          f'{res["refs"]} relocated words (and {res["movw"]} MOVW/MOVT pairs) hold an address in the floating flash, and of '
          f'the words in the literal pools and constant data of every object and library member, ' +
          ('none holds one as a number' if not res['numeric'] else
                                         f'{len(res["numeric"])} are NOT relocated (written as numbers; they would not follow a move)'))
    for b in res['numeric'][:12]: print('   FAIL ' + b)
    if res['numeric']: errors.append('addresses written as numbers')

    # ---- summary and output ---------------------------------------------------------------------------
    used_ram = max([ad + z for ad, z, sec, ob in G.listing_sections(listing) if ad >= 0x20000000] + [zi_end])
    print()
    print('| Region | Range | Used | Free |')
    print('|---|---|---|---|')
    print(f'| vector table (fixed) | 0x{lay.base:08X}..0x{FLOAT_LO - 1:08X} | {FLOAT_LO - lay.base} | 0 |')
    print(f'| application flash (floats) | 0x{FLOAT_LO + a.shift:08X}..0x{FLASH_END - 1:08X} | {image_end - FLOAT_LO - a.shift} | '
          f'{FLASH_END - image_end} |')
    print(f'| RAM, the original\'s RW and ZI (fixed) | 0x{rw.exec:08X}..0x{zi_end - 1:08X} | {zi_end - rw.exec} | 0 |')
    print(f'| RAM for new variables | 0x{zi_end:08X}..0x{RAM_END - 1:08X} | {used_ram - zi_end} | {RAM_END - used_ram} |')
    if errors:
        print(f'FAIL: {"; ".join(errors)}; no package written  ({time.time() - t0:.0f} s)')
        sys.exit(1)
    out = os.path.abspath(a.out)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    header = struct.pack('<HH', version, gc['magic'])
    open(out, 'wb').write(header + built)
    base = os.path.splitext(out)[0]
    width, end, entry = ehex.layout(cfg.image)
    # as tools/fetch_firmware.py writes the original's: the start address is the image's base
    hx = ehex.write(lay.base, built, width=width, end=end, entry=lay.base if entry is not None else None)
    open(base + '.hex', 'w', newline='').write(hx)
    hex_same = hx.encode('ascii') == open(cfg.image, 'rb').read()
    shutil.copy(os.path.join(work, 'link.axf'), base + '.axf')
    open(base + '.map', 'w').write(listing)
    nd = sum(1 for x, y in zip(built, img.data) if x != y) + abs(len(built) - len(img.data))
    obin = open(gc['bin'], 'rb').read()
    print(f'wrote {os.path.relpath(out, ROOT)}: header version {version} + 0x{gc["magic"]:04X}, then 0x{lay.base:08X}-0x{image_end:08X} '
          f'({len(built)} bytes); ' + ('identical to the original ARMStore.bin byte for byte' if header + built == obin else
                                      'identical to the original image (the header differs)' if built == img.data else
                                      f'{nd} bytes of the image differ from the original'))
    print(f'  and {os.path.relpath(base, ROOT)}.hex (the image, Intel HEX' + (', identical to the original\'s A316.hex' if hex_same else '') +
          '), .axf (armlink\'s ELF), .map (its listing)')
    print(f'  version: the running ARM takes the file only when the header version is below 1000, differs from the '
          f'word at 0x08013FFC and has the same hundreds (docs/system/update-packages.md)')
    print(f'PASS  ({time.time() - t0:.0f} s)')


def ehex_image(base, data):
    class I:
        pass
    i = I(); i.base, i.data, i.end = base, data, base + len(data)
    i.bytes = lambda ad, n: data[ad - base:ad - base + n]
    return i


# ---- the library members, with their relocations ---------------------------------------------------
def library_members(tc, gc, lib_listing):
    """{listing object name 'c_w.l(strcpy.o)': Obj} for every member the gate's library link
    placed in the image, extracted from the library home's libraries and read through fromelf."""
    libdir = os.path.join(gc['libhome'], 'ARM', 'ARMCC', 'lib', 'armlib')
    want = {}
    for ad, z, sec, ob in G.listing_sections(lib_listing):
        mm = re.match(r'(\w+\.l)\((.+\.o)\)$', ob)
        if mm: want.setdefault(mm.group(1), set()).add(mm.group(2))
    out = {}
    xdir = os.path.join(MOD, 'libmembers')
    for lib, members in sorted(want.items()):
        d = os.path.join(xdir, lib)
        shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
        path = os.path.join(libdir, lib)
        if not os.path.exists(path): sys.exit(f'modbuild: no {path}')
        rc, log = tc.run('armar', ['-x', tc.inside(path)] + sorted(members), d)
        if rc: sys.exit(f'modbuild: armar -x {lib}: {log}')
        for mb in sorted(members):
            out[f'{lib}({mb})'] = objplace.read_object(tc, os.path.join(d, mb))
    return out


# ---- names for numbers ----------------------------------------------------------------------------
_TOKENS = re.compile(r'(/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|\b(0[xX]0?8[0-9a-fA-F]{6})\b', re.S)


def names_for_numbers(srcs, m, gc):
    """In the copy of the sources, a function's Thumb address written as a number (0x802735b for
    task_measure at 0x0802735A) becomes the function's name, ((int)task_measure), declared at the
    top of the file. Such a number compiles to the same literal word with no relocation, so it
    would not follow its function when the code moves (check 5). Only numbers outside comments
    and strings, with bit 0 set, that are a named function's address + 1 in the floating flash.
    {file: [(number, name)]}"""
    funcs = {r.addr: r.name for r in m.functions if armmap.placeholder(r.name) is None
             and FLOAT_LO <= r.addr < FLASH_END and not any(x <= r.addr < y for x, y in gc['lib_ranges'])}
    out = {}
    for s in srcs:
        if not s.endswith('.c'): continue
        text = open(s, encoding='latin1').read()
        hits = []

        def sub(mm):
            if mm.group(2) is None: return mm.group(0)
            v = int(mm.group(2), 16)
            if v & 1 and v - 1 in funcs:
                hits.append((mm.group(2), funcs[v - 1])); return f'((int){funcs[v - 1]})'
            return mm.group(0)
        new = _TOKENS.sub(sub, text)
        if not hits: continue
        decl = ''.join(f'void {n}(); /* modbuild: names for numbers */\n' for n in sorted({n for _, n in hits}))
        incs = list(re.finditer(r'^\s*#\s*include[^\n]*\n', new, re.M))
        at = incs[-1].end() if incs else 0
        open(s, 'w', encoding='latin1').write(new[:at] + decl + new[at:])
        out[s] = hits
    return out


def numbers_in(ob, o):
    """[(kind, text)] for the words of an object's sections that hold a floating flash address:
    'reloc' a relocated field (its addend in the object, the address in the image), 'number' a word
    written as a number in a literal pool or constant data, 'movw' a MOVW/MOVT pair (a number unless
    relocated)."""
    out = []
    for idx, s in o.sections.items():
        if 'SHF_ALLOC' not in s.flags or s.type == 'SHT_NOBITS' or not s.size: continue
        rels = {r.offset for r in o.relocs.get(idx, []) if r.tname != 'R_ARM_NONE'}
        code = 'SHF_EXECINSTR' in s.flags
        states = objplace.mapping_states(o, idx) if code else []
        b = s.data
        # armcc gives a section that holds a pointer 4-byte alignment, and the pointer a 4-byte
        # boundary: halfword tables (board.c's grid limits) hold pairs that only look like addresses
        for i in (range(0, len(b) - 3, 4) if s.align >= 4 else ()):
            if code and objplace._state_at(states, i) != '$d': continue
            v = struct.unpack_from('<I', b, i)[0] & ~1
            if FLOAT_LO <= v < FLASH_END and i not in rels:
                out.append(('number', f'{ob}({s.name})+0x{i:X}: .word 0x{v:08X} with no relocation'))
        if code:
            for i in range(0, len(b) - 7, 2):
                if objplace._state_at(states, i) != '$t': continue
                h1, h2 = struct.unpack_from('<HH', b, i)
                if h1 & 0xFBF0 != 0xF240: continue                # MOVW, then a MOVT of the same register?
                for j in range(i + 4, min(len(b) - 3, i + 12), 2):
                    k1, k2 = struct.unpack_from('<HH', b, j)
                    if k1 & 0xFBF0 == 0xF2C0 and (k2 >> 8) & 0xF == (h2 >> 8) & 0xF:
                        v = objplace._mov_get(k1, k2) << 16 | objplace._mov_get(h1, h2)
                        if FLOAT_LO <= (v & ~1) < FLASH_END:
                            out.append(('movw', '') if i in rels else ('number', f'{ob}({s.name})+0x{i:X}: MOVW/MOVT 0x{v:08X} with no relocation'))
                        break
    return out


def audit_numbers(objs):
    return [x[1].split(':')[0] for ob, o in sorted(objs.items()) for x in numbers_in(f'{ob}.o', o) if x[0] == 'number']


# ---- 4 and 5 ------------------------------------------------------------------------------------
BRANCH = {'R_ARM_THM_CALL', 'R_ARM_THM_JUMP24', 'R_ARM_THM_JUMP19'}


def target(tname, buf, off, P):
    """The address a relocated field at buf[off] holds, the field placed at P: (address, thumb
    bit) or None for a type that holds no address of its own."""
    if off + 4 > len(buf): return None
    w = struct.unpack_from('<I', buf, off)[0]
    h1, h2 = struct.unpack_from('<HH', buf, off)
    if tname == 'R_ARM_ABS32': return w & ~1, w & 1
    if tname == 'R_ARM_REL32': return (w + P) & 0xFFFFFFFF, 0
    if tname in ('R_ARM_THM_CALL', 'R_ARM_THM_JUMP24'): return (P + 4 + objplace._bl_get(h1, h2)) & 0xFFFFFFFF, 0
    if tname == 'R_ARM_THM_JUMP19': return (P + 4 + objplace._b19_get(h1, h2)) & 0xFFFFFFFF, 0
    return None


def moved_check(lay, img, built, new, objs, gobjs, libmem, lib_listing, changed, order, gc, syms, m):
    """Every section the build shares with the original, compared field by field (module doc, 4),
    and the original's words that hold floating addresses (5)."""
    out = dict(sections=0, bytes=0, same=0, moved=0, fields=0, followed=0, bad=[], absent=[], refs=0, movw=0, numeric=[], weak=0,
               named=0, unchecked=[])
    # where each section was and is: (object name in the listing, section) -> (old, new, size, Obj, index)
    items = []
    for st, o, sec, ad, z in lay.flash:
        if st not in objs: out['absent'].append(f'{st}({sec})'); continue
        nw = new.get((sec, f'{st}.o'))
        idx = next((i for i, s in objs[st].sections.items() if s.name == sec), None)
        if not nw or idx is None: out['absent'].append(f'{st}({sec})'); continue
        # the build's object: the same bytes as the gate's unless changed, its relocations its own
        items.append((f'{st}.o', sec, ad, nw[0][0], z, objs[st], idx, st in changed and any(n.split(' ')[0] == sec for n in changed[st])))
    for ad, z, sec, ob in G.listing_sections(lib_listing):
        if ob not in libmem or not any(x <= ad < y for x, y in gc['lib_ranges']): continue
        cands = [i for i, s in libmem[ob].sections.items() if s.name == sec]
        nw = new.get((sec, ob))
        if len(cands) != 1 or not nw:
            out['absent'].append(f'{ob}({sec})'); continue
        items.append((ob, sec, ad, nw[0][0], z, libmem[ob], cands[0], False))
    # the region table and the compressed RW data after it
    tb_new = new.get(('Region$$Table', 'anon$$obj.o'), [(None, 0)])[0]
    o_tsize = lay.table[1] - lay.table[0]
    spans = [(x[2], x[4], x[3]) for x in items if x[4]]
    if tb_new[0] is not None:
        spans.append((lay.table[0], o_tsize, tb_new[0]))
        spans.append((lay.table[1], lay.end - lay.table[1], tb_new[0] + tb_new[1]))       # the compressed RW data
    # changed sections: where they start is known; a place inside them moves with its name
    cspans = []
    for st, o, sec, ad, z in lay.flash:
        if st in changed and any(n.split(' ')[0] == sec for n in changed[st]) and (sec, f'{st}.o') in new:
            cspans.append((ad, z, new[(sec, f'{st}.o')][0][0]))
    spans += [c for c in cspans]
    spans.sort()
    starts = [s[0] for s in spans]

    def translate(x):
        i = bisect.bisect_right(starts, x) - 1
        if i >= 0 and spans[i][0] <= x < spans[i][0] + spans[i][1]: return spans[i][2] + x - spans[i][0]
        for j in (i, i - 1):
            if j >= 0 and x == spans[j][0] + spans[j][1]: return spans[j][2] + spans[j][1]
        return x                                                  # fixed: RAM, peripherals, the boot area

    NB = lambda ad, n: built[ad - lay.base:ad - lay.base + n]
    OB = lambda ad, n: img.data[ad - lay.base:ad - lay.base + n]
    for ob, sec, old, nwa, z, o, idx, chg in items:
        s = o.sections[idx]
        rels = sorted(o.relocs.get(idx, []), key=lambda r: r.offset)
        code = 'SHF_EXECINSTR' in s.flags
        states = objplace.mapping_states(o, idx) if code else []
        roffs = set()
        for r in rels:
            if r.tname != 'R_ARM_NONE': roffs.update(range(r.offset, r.offset + 4))
        ob_ = OB(old, z)
        if chg: continue
        # 4: moved, not changed
        nb = NB(nwa, z)
        out['sections'] += 1; out['bytes'] += z
        if nwa != old: out['moved'] += z
        if len(nb) != z: out['bad'].append(f'{ob}({sec}): past the end of the build'); continue
        diff = {i for i in range(z) if nb[i] != ob_[i]}
        out['same'] += z - len(diff)
        covered = set()
        for r in rels:
            if r.tname == 'R_ARM_NONE': continue
            if r.tname in ('R_ARM_THM_MOVW_ABS_NC', 'R_ARM_THM_MOVT_ABS'):
                ov, nv = objplace._mov_get(*struct.unpack_from('<HH', ob_, r.offset)), objplace._mov_get(*struct.unpack_from('<HH', nb, r.offset))
                # checked as a pair through the whole value: the MOVT carries the high half
                if r.tname == 'R_ARM_THM_MOVW_ABS_NC':
                    mt = next((x for x in rels if x.tname == 'R_ARM_THM_MOVT_ABS' and x.sname == r.sname and x.offset > r.offset), None)
                    if mt:
                        oh = objplace._mov_get(*struct.unpack_from('<HH', ob_, mt.offset)); nh = objplace._mov_get(*struct.unpack_from('<HH', nb, mt.offset))
                        out['fields'] += 1
                        o32, n32 = oh << 16 | ov, nh << 16 | nv
                        if (n32 & ~1) == translate(o32 & ~1) and n32 & 1 == o32 & 1:
                            out['followed'] += translate(o32 & ~1) != (o32 & ~1)
                        else: out['bad'].append(f'{ob}({sec})+0x{r.offset:X} MOVW/MOVT {r.sname}: 0x{n32:08X}, the original\'s 0x{o32:08X} moves to 0x{translate(o32 & ~1):08X}')
                        covered.update(range(r.offset, r.offset + 4)); covered.update(range(mt.offset, mt.offset + 4))
                continue
            ws = o.symbols.get(r.sym)
            if ws is not None and ws.bind == 'Wk' and ws.sec == 'Ref' and r.sname not in syms:
                # a weak reference to something the link does not have: armlink writes the same
                # stand-in instruction wherever it is (exit's _call_atexit_fns)
                out['fields'] += 1; out['weak'] += 1
                covered.update(range(r.offset, r.offset + 4))
                if nb[r.offset:r.offset + 4] != ob_[r.offset:r.offset + 4]:
                    out['bad'].append(f'{ob}({sec})+0x{r.offset:X} {r.tname} {r.sname} (weak, not linked): the bytes differ')
                continue
            ot, nt = target(r.tname, ob_, r.offset, old + r.offset), target(r.tname, nb, r.offset, nwa + r.offset)
            if ot is None:
                if any(i in diff for i in range(r.offset, r.offset + 4)):
                    out['bad'].append(f'{ob}({sec})+0x{r.offset:X} {r.tname} {r.sname}: differs, and the type is not decoded')
                continue
            out['fields'] += 1
            covered.update(range(r.offset, r.offset + 4))
            if r.tname == 'R_ARM_ABS32' and FLOAT_LO <= nt[0] < FLASH_END: out['refs'] += 1
            # a named target in a changed section (a function after one that grew) moves with its
            # name, not with the section: the name's new address, plus the same offset from it
            row = m.by_name.get(r.sname) if ws is not None and ws.type != 'Sect' else None
            inchg = next((c for c in cspans if c[0] <= ot[0] < c[0] + c[1]), None)
            if inchg and row is not None and r.sname in syms:
                want = (syms[r.sname] & ~1) + (ot[0] - row.addr); out['named'] += 1
            elif inchg and ot[0] != inchg[0]:
                out['unchecked'].append(f'{ob}({sec})+0x{r.offset:X} {r.tname} {r.sname}: into a changed section'); continue
            else:
                want = translate(ot[0])
            if nt == (want, ot[1]):
                out['followed'] += want != ot[0]
            else:
                out['bad'].append(f'{ob}({sec})+0x{r.offset:X} {r.tname} {r.sname}: 0x{nt[0]:08X}, the original\'s 0x{ot[0]:08X} moves to 0x{want:08X}')
        for i in sorted(diff - covered)[:4]:
            out['bad'].append(f'{ob}({sec})+0x{i:X}: 0x{nb[i]:02X}, original 0x{ob_[i]:02X}, not in a relocated field')
    # 5: in every section of the build's objects and of the library members, a word that holds a
    # floating flash address must be a relocated field
    allobjs = {f'{st}.o': o for st, o in objs.items()}
    allobjs.update(libmem)
    for ob, o in allobjs.items():
        for x in numbers_in(ob, o):
            if x[0] == 'movw': out['movw'] += 1
            elif x[0] == 'number': out['numeric'].append(x[1])
    # the region table: each entry's addresses follow; the compressed RW data is the same bytes
    if tb_new[0] is not None:
        ot_ = OB(lay.table[0], o_tsize); nt_ = NB(tb_new[0], o_tsize)
        out['sections'] += 1; out['bytes'] += o_tsize
        for i in range(0, o_tsize, 4):
            ow, nw = struct.unpack_from('<I', ot_, i)[0], struct.unpack_from('<I', nt_, i)[0]
            out['fields'] += 1
            if (nw & ~1) == translate(ow & ~1) and nw & 1 == ow & 1: out['followed'] += translate(ow & ~1) != (ow & ~1)
            else: out['bad'].append(f'Region$$Table+0x{i:X}: 0x{nw:08X}, the original\'s 0x{ow:08X} moves to 0x{translate(ow & ~1):08X}')
        n = lay.end - lay.table[1]
        ocomp, ncomp = OB(lay.table[1], n), NB(tb_new[0] + tb_new[1], n)
        out['sections'] += 1; out['bytes'] += n
        if ocomp != ncomp and not changed: out['bad'].append('the compressed RW data differs from the original\'s')
        out['same'] += sum(1 for x, y in zip(ocomp, ncomp) if x == y) + o_tsize - sum(1 for x, y in zip(ot_, nt_) if x != y)
    return out


if __name__ == '__main__':
    main()
