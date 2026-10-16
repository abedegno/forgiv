"""The modding build: compile every source and link a working DSPStore.bin with TI's linker, with
the application's code and data free to move and change size. It is not compared with the
original; tools/gate.py stays the proof that the sources are the original.

    python3 tools/modbuild.py [--src DIR | --patch FILE ...] [--shift N] [--out PATH] [--native]

--src DIR     build the sources in DIR (a copy of src/, same layout, DIR/include first on the
              include path) in place of src/
--patch FILE  copy src/ to build/mod/tree, apply FILE there with `patch -p1` (paths in the
              patch are relative to the d316 directory, a/src/... b/src/...) and build that;
              may be given more than once. src/ itself is never changed.
--shift N     move the floating application flash and the ramfunc run region up by N words
              (a layout audit: everything must still link and every check pass)
--out PATH    where to write the image (default build/mod/DSPStore.bin)
--native      link with the native lnk2000 (18.12.8) instead of [gate] linker's (16.9.6 under
              Wine); both give the same image

What is fixed and what floats (docs in README.md, "The modding build"):

  fixed   0x3E8000        version (app_header.c): the ARM compares the file's first word
          0x3E8004        app_entry, where boot_main jumps (LB 0x3E8004)
          0x3F3FFC        app_valid = 0xAA, the last word the ARM's 'D' blocks write
          0x3F4000..      sector A: the boot loader (boot_main, boot_update, its RAM routine
                          loaded at 0x3F5004 and run at 0x0400), codestart, the CSM words. The
                          update path never writes sector A, so these words must come out as
                          the original's; the build checks it.
          RAM             every file's .ebss at its original address (the map's RAM names,
                          views like ram_C000 and the library's data depend on it), the stack
                          at 0xE390, the peripheral registers and PieVectTable at 0x0D00
  floats  0x3E8008..0x3F3FFB (sectors D, C, B): .text, the ramfuncs' load image, .cinit,
          .econst and .switch, in that order, in the original's order inside each; anything
          new goes at the end of its group
          0x8000..0xBFFF (L0..L4): the ramfuncs' run addresses; init_system copies them with
          the linker's RamfuncsLoadStart/RamfuncsLoadSize/RamfuncsRunStart
          0x10000..0x13FFF (L7, L8): .ebss of a new source file

Checks (any failure gives exit status 1, and no image is written):
  1. link: no unresolved symbol; nothing placed by the map at an address that floats; every
     function of map/functions.tsv in the image.
  2. fixed words: the version, app_valid 0xAA, sector A word for word the original's, the
     ramfunc copy symbols and regions, each .ebss variable at its map address and no file's
     .ebss larger than the gate's.
  3. objects: each section compared with the gate's object of the same file (build/gate/src,
     from the last tools/gate.py run): identical, or listed as changed by the mod.
  4. moved, not changed: every section the mod did not change is compared with the original
     image at its original address; words may differ only in relocated fields, and each such
     field must point to the same thing moved (the original's target, translated by the move
     of the section that holds it).
  5. numbers: every call, long branch or MOVL XARn,# in the original's code that holds an address
     in a floating region, and every table pointer to a map address, is a relocated field (one
     written as a number would not follow a move).
With nothing changed the image comes out as the original, word for word: the floating layout
keeps the original's order. --shift shows that nothing depends on that.
Output: the image (131072 bytes: 0x3E8000..0x3F7FFF as big-endian 16-bit words, holes 0xFFFF,
the format of the package's DSPStore.bin), the linker's map and XML beside it (DSPStore.map,
DSPStore.xml; tools/emudiff.py reads the XML), the link in build/mod/link, and a summary with the
free space of each region."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d316  # noqa: E402  (first: it loads the standard library's dis, which tools/dis.py hides)
import argparse, bisect, collections, concurrent.futures, re, shutil, struct, subprocess, time  # noqa: E402
import xml.etree.ElementTree as ET  # noqa: E402
import libfind, match  # noqa: E402

MOD = os.path.join(d316.BUILD, 'mod')
GATE_SRC = os.path.join(d316.BUILD, 'gate', 'src')

# the regions (word addresses); PAGE 0 program, PAGE 1 data
APP_LO, APP_HI = 0x3E8008, 0x3F3FFC          # sectors D, C, B less the header and app_valid words
SECTOR_A = 0x3F4000
RAMFUNC_LO, RAMFUNC_HI = 0x8000, 0xC000      # L0..L4
RAMDATA_LO, RAMDATA_HI = 0xC000, 0x10000     # L5, L6: the original's variables and the stack
RAMNEW_LO, RAMNEW_HI = 0x10000, 0x14000      # L7, L8: new files' variables
STACK = 0xE390
LINKER_DEFINED = {'_RamfuncsLoadStart', '_RamfuncsLoadSize', '_RamfuncsRunStart',
                  '_BootRamfuncsLoadStart', '_BootRamfuncsLoadEnd', '_BootRamfuncsRunStart'}
CINIT_LO, CINIT_HI = 0x3F336F, 0x3F3B43      # the original's .cinit (for the move check)


def stem(p):
    return os.path.splitext(os.path.basename(p))[0]


def tree_sources(srcdir):
    out, inc = [], os.path.join(srcdir, 'include')
    for d, dirs, files in os.walk(srcdir):
        if os.path.commonpath([d, inc]) == inc: dirs[:] = []; continue
        dirs.sort()
        out += [os.path.join(d, n) for n in sorted(files) if n.endswith(('.c', '.asm'))]
    seen = {}
    for p in out:
        if stem(p) in seen: raise d316.ToolError('two sources with the stem %s' % stem(p))
        seen[stem(p)] = p
    return out


def compile_all(srcs, incdir, objdir):
    """compile every source, several Wine jobs side by side; returns {stem: obj path}"""
    def one(chunk):
        out = {}
        with d316.Wine() as w:
            for src in chunk:
                work = os.path.join(objdir, stem(src))
                shutil.rmtree(work, ignore_errors=True); os.makedirs(work)
                extra = ['-I' + incdir] + match.cflags_of(src)
                out[stem(src)] = d316.compile_c(w, src, work, cgt=match.cgt_of(src), extra=extra)
        return out
    n = 4
    chunks = [srcs[i::n] for i in range(n)]
    objs = {}
    with concurrent.futures.ThreadPoolExecutor(n) as ex:
        for r in ex.map(one, chunks): objs.update(r)
    return objs


def q(s):
    return '"%s"' % s


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    g = ap.add_mutually_exclusive_group()
    g.add_argument('--src', help='a copy of src/ to build')
    g.add_argument('--patch', action='append', default=[], help='a patch to apply to a copy of src/')
    ap.add_argument('--shift', type=lambda s: int(s, 0), default=0)
    ap.add_argument('--out', default=os.path.join(MOD, 'DSPStore.bin'))
    ap.add_argument('--native', action='store_true', help="link with the native lnk2000, not the configured one under Wine")
    a = ap.parse_args()
    t0 = time.time()
    errors, notes = [], []

    # ---- the tree --------------------------------------------------------------------------
    os.makedirs(MOD, exist_ok=True)
    srcdir = d316.SRC
    if a.src:
        srcdir = os.path.abspath(a.src)
    elif a.patch:
        srcdir = os.path.join(MOD, 'tree', 'src')
        shutil.rmtree(os.path.join(MOD, 'tree'), ignore_errors=True)
        shutil.copytree(d316.SRC, srcdir)
        for p in a.patch:
            r = subprocess.run(['patch', '-p1', '--no-backup-if-mismatch', '-i', os.path.abspath(p)],
                               cwd=os.path.join(MOD, 'tree'), capture_output=True, text=True)
            print('patch %s:\n  %s' % (os.path.relpath(p), (r.stdout + r.stderr).strip().replace('\n', '\n  ')))
            if r.returncode: print('FAIL: the patch does not apply'); sys.exit(1)
    srcs = tree_sources(srcdir)
    print('modbuild: %d sources from %s' % (len(srcs), os.path.relpath(srcdir, d316.ROOT)))
    work = os.path.join(MOD, 'link')
    shutil.rmtree(work, ignore_errors=True); os.makedirs(work)
    if os.path.exists(a.out): os.remove(a.out)

    # ---- compile ---------------------------------------------------------------------------
    try:
        objpaths = compile_all(srcs, os.path.join(srcdir, 'include'), os.path.join(MOD, 'obj'))
    except d316.ToolError as e:
        print('compile: FAIL\n%s' % e); d316.stop_wine(); sys.exit(1)
    print('compile: %d objects' % len(objpaths))

    # ---- where each section was in the original (the map, as the gate places it) -------------
    frows, drows = match.map_rows()
    objs, orig = {}, {}                 # orig[(stem, section)] = (load, run) in the original, or None
    symdefs, defined = {}, set()
    for st in sorted(objpaths):
        o = objs[st] = d316.Obj(objpaths[st])
        placements, sd, targets, pnotes, unres = match.plan(o, frows, drows)
        for name, base, page in placements:
            run = load = base
            for r, sname, off, ld in targets:
                if sname == name and ld != r.run: load = ld - off; break
            if match.PARK <= base < 0x3D0000: load = run = None      # no map address: new
            orig[(st, name)] = (load, run)
        for p in pnotes:
            if '.ebss' in p and 'wants' in p: errors.append('%s: %s' % (st, p))
        defined.update(o.defined())
        symdefs.update(sd)

    # ---- the library (what the original links, as the gate finds it) --------------------------
    lib, found = libfind.find()
    libdir, rtsdir = os.path.dirname(lib), libfind.extract(lib)[1]
    members = {}
    lib_orig = {}                       # (member, section) -> original address
    for f in found:
        members.setdefault(f['member'], d316.Obj(os.path.join(rtsdir, f['member'])))
        lib_orig[(f['member'], f['section'])] = f['addr']
    lib_ram = []                        # (member, section, address): the library's data, fixed
    from_image = {}
    for f in found:
        if f['kind'] != 'text': continue
        o = members[f['member']]
        for sym, vals in d316.derive_bases(o.sections[f['section']], f['addr']).items():
            v = collections.Counter(vals).most_common(1)[0][0]
            if sym in o.sections and o.sections[sym].kind == 'bss':
                if (f['member'], sym) not in [(m, s) for m, s, _ in lib_ram]: lib_ram.append((f['member'], sym, v))
            elif sym not in o.sections and sym != '(internal)':
                from_image[sym] = v
    order = d316.cfg()['gate'].get('cinit_order', [])
    for e in order:
        if e.startswith('rts:') and e[4:] not in members:
            m = e[4:]
            o = members[m] = d316.Obj(os.path.join(rtsdir, m))
            for n, (sn, v) in o.defined().items():
                if o.sections[sn].kind == 'bss' and n in from_image:
                    lib_ram.append((m, sn, from_image[n] - v))
    for m, o in members.items():
        for n in o.defined(): symdefs.pop(n, None)

    # symbols the objects use and nobody defines: from the map, but only where nothing moves
    for n in list(symdefs):
        v = symdefs[n]
        if n in defined or n in LINKER_DEFINED:
            del symdefs[n]; continue
        if APP_LO - 8 <= v < SECTOR_A or RAMFUNC_LO <= v < RAMFUNC_HI:
            # an address that floats: the linker must resolve it (a library member or an error)
            del symdefs[n]
            notes.append('%s: not defined from the map (0x%X floats); left to the linker' % (n, v))

    # ---- sort the sections into the groups ---------------------------------------------------
    fixed = []                          # (spec, out name, load, run, page)
    text, ramf, cinit, econst = [], [], [], []
    ebss_fixed, ebss_new = [], []
    boot_ram = None
    version = app_entry = app_valid = None
    big = 1 << 40
    for st, o in objs.items():
        for name, sec in o.sections.items():
            if sec.kind not in ('text', 'data', 'bss') or not sec.size or name.startswith('.debug'): continue
            load, run = orig.get((st, name), (None, None))
            spec = '%s(%s)' % (q(objpaths[st]), name)
            key = load if load is not None else big
            if name == 'version': version = spec
            elif name == 'app_valid': app_valid = spec
            elif name == '.text:_app_entry': app_entry = spec
            elif name.startswith('.cinit'): cinit.append((st, spec))
            elif sec.kind == 'bss':
                (ebss_fixed if load is not None and RAMDATA_LO <= load < RAMDATA_HI else ebss_new).append((st, name, spec, load))
            elif name.startswith('ramfuncs') and run is not None and run < RAMFUNC_LO:
                boot_ram = (spec, load, run)
            elif load is not None and load >= SECTOR_A:
                fixed.append((spec, re.sub(r'\W', '_', '%s_%s' % (st, name)), load, load, 0))
            elif name.startswith('ramfuncs'): ramf.append((key, st, name, spec))
            elif name.startswith(('.econst', '.switch')): econst.append((key, st, name, spec))
            elif sec.kind == 'text': text.append((key, st, name, spec))
            else:
                errors.append('%s: section %s has no place in the modding build' % (st, name))
    for f in found:
        spec = 'rts2800_fpu32.lib<%s>(%s)' % (f['member'], f['section'])
        if f['kind'] == 'text': text.append((f['addr'], 'rts:' + f['member'], f['section'], spec))
        else: econst.append((f['addr'], 'rts:' + f['member'], f['section'], spec))
    for grp in (text, ramf, econst): grp.sort(key=lambda x: (x[0], x[1], x[2]))
    for need, what in ((version, 'version'), (app_valid, 'app_valid'), (app_entry, 'app_entry'), (boot_ram, 'boot_flash_loader')):
        if need is None: errors.append('no %s section (the fixed layout needs it)' % what)

    # .cinit: the original's file order, then any other file's
    cinit_spec = []
    by_stem = collections.defaultdict(list)
    for st, spec in cinit: by_stem[st].append(spec)
    for e in order:
        if e.startswith('rts:'): cinit_spec.append('rts2800_fpu32.lib<%s>(.cinit)' % e[4:])
        else: cinit_spec += by_stem.pop(e, [])
    for st in sorted(by_stem): cinit_spec += by_stem[st]

    # ---- the command file ---------------------------------------------------------------------
    sh = a.shift

    def write_cmd(path, loads):
        """loads None: the application's groups in one GROUP (the first pass, to find where each
        starts); else {group: load address} for the second pass, which gives the ramfuncs their
        run addresses (a GROUP member cannot have a run address of its own)."""
        with open(path, 'w') as f:
            f.write('/* the D316 modding build: written by tools/modbuild.py */\n')
            f.write('-a\n-o mod.out\n-m mod.map\n--xml_link_info=mod.xml\n--rom_model\n--stack_size=0x400\n')
            f.write('--disable_auto_rts\n--disable_clink\n')
            f.write('-i %s\n-l rts2800_fpu32.lib\n' % q(libdir))
            for m, o in sorted(members.items()):
                gl = sorted(n for n, (sn, v) in o.defined().items() if o.sections[sn].kind == 'text') or sorted(o.defined())
                if gl: f.write('-u %s\n' % gl[0])
            for st in sorted(objpaths): f.write('%s\n' % q(objpaths[st]))
            f.write('\nMEMORY\n{\nPAGE 0:\n')
            f.write('  BOOTRUN  : origin = 0x000400, length = 0x000400   /* M1: the boot loader\'s RAM routine runs here */\n')
            f.write('  RAMFUNC  : origin = 0x%06X, length = 0x%06X   /* L0..L4: the ramfuncs run here */\n' % (RAMFUNC_LO + sh, RAMFUNC_HI - RAMFUNC_LO - sh))
            f.write('  VERSION  : origin = 0x3E8000, length = 0x000004   /* the version word (the ARM reads it) */\n')
            f.write('  ENTRY    : origin = 0x3E8004, length = 0x000004   /* app_entry: boot_main jumps here */\n')
            f.write('  APPFLASH : origin = 0x%06X, length = 0x%06X   /* sectors D, C, B: the application */\n' % (APP_LO + sh, APP_HI - APP_LO - sh))
            f.write('  APPVALID : origin = 0x3F3FFC, length = 0x000004   /* app_valid 0xAA */\n')
            f.write('  BOOT     : origin = 0x3F4000, length = 0x003F80   /* sector A: the boot loader, never rewritten */\n')
            f.write('  CSMRSVD  : origin = 0x3F7F80, length = 0x000076\n')
            f.write('  BEGIN    : origin = 0x3F7FF6, length = 0x000002   /* codestart */\n')
            f.write('  CSMPWL   : origin = 0x3F7FF8, length = 0x000008\n')
            f.write('  RESET    : origin = 0x3FFFC0, length = 0x000002\n')
            f.write('PAGE 1:\n')
            f.write('  RAMDATA  : origin = 0x%06X, length = 0x%06X   /* L5, L6: the original\'s variables, the stack */\n' % (RAMDATA_LO, RAMDATA_HI - RAMDATA_LO))
            f.write('  RAMNEW   : origin = 0x%06X, length = 0x%06X   /* L7, L8: variables of new files */\n}\n\n' % (RAMNEW_LO, RAMNEW_HI - RAMNEW_LO))
            f.write('SECTIONS\n{\n')
            f.write('  /* fixed */\n')
            f.write('  version   : { %s } > VERSION, PAGE = 0\n' % version)
            f.write('  app_entry : { %s } > ENTRY, PAGE = 0\n' % app_entry)
            f.write('  app_valid : { %s } > APPVALID, PAGE = 0\n' % app_valid)
            for spec, out, load, run, page in sorted(fixed, key=lambda x: x[2]):
                f.write('  %s : { %s } load = 0x%X, PAGE = %d\n' % (out, spec, load, page))
            f.write('  boot_ramfuncs : { %s } load = 0x%X, run = 0x%X, PAGE = 0,\n' % boot_ram)
            f.write('      LOAD_START(_BootRamfuncsLoadStart), LOAD_END(_BootRamfuncsLoadEnd), RUN_START(_BootRamfuncsRunStart)\n')
            f.write('  .reset : > RESET, PAGE = 0, TYPE = DSECT\n')
            f.write('\n  /* the application: floats in APPFLASH, in this order */\n')
            at = lambda g: ('load = 0x%X' % loads[g]) if loads else ''
            if not loads: f.write('  GROUP : > APPFLASH, PAGE = 0\n  {\n')
            f.write('    .text : %s\n    {\n' % (at('.text') + (', PAGE = 0' if loads else '')))
            for x in text: f.write('      %s\n' % x[3])
            f.write('      *(.text)\n    }\n')
            if loads:
                f.write('    ramfuncs : load = 0x%X, run = RAMFUNC, PAGE = 0,\n' % loads['ramfuncs'])
                f.write('      LOAD_START(_RamfuncsLoadStart), LOAD_SIZE(_RamfuncsLoadSize), RUN_START(_RamfuncsRunStart)\n    {\n')
            else:
                f.write('    ramfuncs : LOAD_START(_RamfuncsLoadStart), LOAD_SIZE(_RamfuncsLoadSize), RUN_START(_RamfuncsRunStart)\n    {\n')
            for x in ramf: f.write('      %s\n' % x[3])
            f.write('    }\n')
            f.write('    .cinit : %s\n    {\n' % (at('.cinit') + (', PAGE = 0' if loads else '')))
            for s_ in cinit_spec: f.write('      %s\n' % s_)
            f.write('      *(.cinit)\n    }\n')
            f.write('    .econst : %s\n    {\n' % (at('.econst') + (', PAGE = 0' if loads else '')))
            for x in econst: f.write('      %s\n' % x[3])
            f.write('      *(.econst)\n      *(.switch)\n    }\n')
            if not loads: f.write('  }\n')
            f.write('\n  /* RAM: each file\'s variables where the original has them */\n')
            for st, name, spec, load in ebss_fixed:
                f.write('  %s : { %s } load = 0x%X, PAGE = 1\n' % (re.sub(r'\W', '_', '%s_%s' % (st, name)), spec, load))
            for m, sn, v in lib_ram:
                f.write('  %s : { rts2800_fpu32.lib<%s>(%s) } load = 0x%X, PAGE = 1\n' % (re.sub(r'\W', '_', 'rts_%s_%s' % (m[:-4], sn)), m, sn, v))
            f.write('  .stack : load = 0x%X, PAGE = 1\n' % STACK)
            f.write('  .ebss : { %s *(.ebss) } > RAMNEW, PAGE = 1\n' % ' '.join(s_ for _, _, s_, _ in ebss_new))
            f.write('  .esysmem : > RAMNEW, PAGE = 1\n  .cio : > RAMNEW, PAGE = 1\n')
            f.write('}\n\n/* from the map: peripheral registers, RAM views, the boot ROM\'s flash API */\n')
            for n, v in sorted(symdefs.items()): f.write('%s = 0x%X;\n' % (n, v))

    if errors:
        for e in errors: print('  ' + e)
        print('FAIL'); d316.stop_wine(); sys.exit(1)

    linker = 'native' if a.native else d316.cfg()['gate'].get('linker', 'cgt')

    def link(tag):
        if linker == 'native':
            p = d316.run([d316.native('lnk2000'), 'mod.cmd'], cwd=work, check=False)
        else:
            with d316.Wine() as w:
                p = w.run([d316.cgt_bin('lnk2000'), 'mod.cmd'], cwd=work, check=False)
        log = (p.stdout + p.stderr).strip()
        if p.returncode or not os.path.exists(os.path.join(work, 'mod.out')):
            print('link (%s): FAIL\n%s' % (tag, log)); d316.stop_wine(); sys.exit(1)
        return log

    cmd = os.path.join(work, 'mod.cmd')
    write_cmd(cmd, None)
    link('first pass')
    x = read_xml(os.path.join(work, 'mod.xml'))
    loads = {g: x['groups'][g][0] for g in ('.text', 'ramfuncs', '.cinit', '.econst')}
    shutil.copy(cmd, os.path.join(work, 'pass1.cmd'))
    os.remove(os.path.join(work, 'mod.out'))
    write_cmd(cmd, loads)
    log = link('second pass')
    x = read_xml(os.path.join(work, 'mod.xml'))
    # a wildcard that matches nothing (*(.text) with nothing new) is not worth a line
    warns = [w for w in re.split(r'\n(?=")', log) if w.strip() and 'matches no sections' not in w]
    syms, comps, groups = x['syms'], x['comps'], x['groups']
    print('link: lnk2000 %s, two passes, %d sections placed, no unresolved symbols%s' % (
          'native' if linker == 'native' else 'under Wine', len(comps), '' if not warns else ', %d warnings' % len(warns)))
    for w in warns: print('  ' + w.replace('\n', ' '))

    # ---- the image ------------------------------------------------------------------------------
    hexcmd = os.path.join(work, 'hex.cmd')
    with open(hexcmd, 'w') as f:
        f.write('mod.out\n-b\n-image\n-order MS\n-romwidth 16\n-fill 0xFFFF\n')
        f.write('ROMS\n{\n  FLASH: origin = 0x%X, length = 0x%X, romwidth = 16, files = { mod.bin }\n}\n'
                % (2 * d316.BASE, 2 * (d316.END - d316.BASE)))
    d316.run([d316.native('hex2000'), 'hex.cmd'], cwd=work)
    built = open(os.path.join(work, 'mod.bin'), 'rb').read()
    if len(built) != 2 * 0x10000:
        errors.append('the image is %d bytes, not 131072' % len(built))
        built = (built + b'\xff' * 0x20000)[:0x20000]
    img = struct.unpack('>65536H', built)
    W = lambda k: img[k - d316.BASE]
    orig_img = d316.image()

    # ---- 1. link: every function of the map is there -----------------------------------------
    comp_by_key = {c['key']: c for c in comps}
    missing = []
    for r in frows.values():
        if r['kind'] == 'rts':
            if ('rts:' + (d316.note(r, 'member') or ''), d316.note(r, 'section') or '.text') not in comp_by_key \
               and not any(k[0] == 'rts:' + (d316.note(r, 'member') or '') for k in comp_by_key):
                missing.append(r['name'])
        elif d316.note(r, 'section'):
            if not any(c['name'] == d316.note(r, 'section') for c in comps): missing.append(r['name'])
        elif '_' + r['name'] not in syms and r['name'] not in syms:
            missing.append(r['name'])
    print('1. link: %d of %d functions of map/functions.tsv in the image%s' % (
          len(frows) - len(missing), len(frows), '' if not missing else '; MISSING: ' + ', '.join(missing[:10])))
    if missing: errors.append('functions missing')
    for n in notes: print('  note: ' + n)

    # ---- 2. the fixed words --------------------------------------------------------------------
    fx = []
    ver = W(0x3E8000)
    fx.append(('version word 0x3E8000 = 0x%04X (%d)' % (ver, ver), ver != 0xFFFF))
    fx.append(('app_entry at 0x3E8004', comp_by_key.get(('grid_support', '.text:_app_entry'), {}).get('load') == 0x3E8004))
    fx.append(('app_valid 0x3F3FFC = 0x%04X' % W(0x3F3FFC), W(0x3F3FFC) == 0x00AA))
    bad_a = [k for k in range(SECTOR_A, d316.END) if W(k) != orig_img[k - d316.BASE]]
    fx.append(('sector A 0x3F4000..0x3F7FFF (boot loader, codestart, CSM) word for word the original%s'
               % ('' if not bad_a else ': %d words differ, the first at 0x%X' % (len(bad_a), bad_a[0])), not bad_a))
    rf = groups.get('ramfuncs')
    fx.append(('ramfuncs: load 0x%X, run 0x%X, 0x%X words; RamfuncsLoadStart/RunStart/LoadSize agree' % (rf[0], rf[1], rf[2]),
               syms.get('_RamfuncsLoadStart') == rf[0] and syms.get('_RamfuncsRunStart') == rf[1] == RAMFUNC_LO + sh
               and syms.get('_RamfuncsLoadSize') == rf[2] and rf[1] + rf[2] <= RAMFUNC_HI))
    br = groups.get('boot_ramfuncs')
    fx.append(('boot loader RAM routine: load 0x%X, run 0x%X (BootRamfuncs* agree)' % (br[0], br[1]),
               br[0] == 0x3F5004 and br[1] == 0x400 and syms.get('_BootRamfuncsLoadStart') == br[0]
               and syms.get('_BootRamfuncsLoadEnd') == br[0] + br[2] and syms.get('_BootRamfuncsRunStart') == 0x400))
    pv = syms.get('_PieVectTable'); pvi = syms.get('_PieVectTableInit')
    fx.append(('PieVectTable at 0x0D00 (fixed), PieVectTableInit at 0x%X, 256 words' % (pvi or 0),
               pv == 0xD00 and pvi is not None and comp_by_key.get(('F2806x_PieVect', '.econst:_PieVectTableInit'), {}).get('size') == 256))
    # RAM: each variable at its map address, no .ebss larger than the gate's
    moved_vars, grown = [], []
    for st, name, spec, load in ebss_fixed:
        o = objs[st]
        for n, (sn, v) in o.defined().items():
            if sn != name: continue
            row = drows.get(n[1:] if n.startswith('_') else n)
            if row and syms.get(n) != row['addr']: moved_vars.append('%s 0x%X (map 0x%X)' % (n, syms.get(n, 0), row['addr']))
        g = os.path.join(GATE_SRC, st, st + '.obj')
        if os.path.exists(g):
            gs = d316.Obj(g).sections.get(name)
            if gs and o.sections[name].size > gs.size: grown.append('%s %d words (gate %d)' % (st, o.sections[name].size, gs.size))
    fx.append(('RAM: every variable of the fixed .ebss sections at its map address%s'
               % ('' if not moved_vars else ': MOVED ' + ', '.join(moved_vars[:6])), not moved_vars))
    fx.append(('RAM: no file\'s .ebss larger than in the gate%s'
               % ('' if not grown else ': GREW ' + ', '.join(grown) + ' (new variables go in a new source file)'), not grown))
    bad_fx = [t for t, ok in fx if not ok]
    print('2. fixed: %s' % ('all as required' if not bad_fx else '%d FAIL' % len(bad_fx)))
    for t, ok in fx: print('   %s %s' % ('ok  ' if ok else 'FAIL', t))
    if bad_fx: errors.append('fixed layout')

    # ---- 3. objects against the gate's ------------------------------------------------------------
    changed, new_files, same = {}, [], 0
    if not os.path.isdir(GATE_SRC):
        print('3. objects: no build/gate/src (run tools/gate.py first); not compared')
    else:
        for st, o in sorted(objs.items()):
            g = os.path.join(GATE_SRC, st, st + '.obj')
            if not os.path.exists(g): new_files.append(st); continue
            go = d316.Obj(g)
            names = {n for n, s_ in list(o.sections.items()) + list(go.sections.items())
                     if s_.kind in ('text', 'data', 'bss') and s_.size and not n.startswith('.debug')}
            for n in sorted(names):
                s1, s2 = o.sections.get(n), go.sections.get(n)
                if s1 is None or s2 is None or s1.data != s2.data or s1.size != s2.size or \
                   [(r[0], r[1], r[2]) for r in s1.relocs] != [(r[0], r[1], r[2]) for r in s2.relocs]:
                    changed.setdefault(st, []).append(n + ('' if s1 and s2 else ' (new)' if s1 else ' (removed)'))
                else:
                    same += 1
        nch = sum(len(v) for v in changed.values())
        print('3. objects: %d sections identical to the gate\'s objects, %d changed%s' % (
              same, nch, ', %d new files' % len(new_files) if new_files else ''))
        for st, ns in sorted(changed.items()):
            print('   changed: %s: %s' % (st, ', '.join(ns)))
            go = d316.Obj(os.path.join(GATE_SRC, st, st + '.obj'))
            for n in ns:
                s1, s2 = objs[st].sections.get(n), go.sections.get(n)
                c = comp_by_key.get((st, n))
                if not (s1 and s2 and c) or s1.kind == 'bss': continue
                if s1.size != s2.size:
                    print('     %s: %d words, %d in the original (what follows it moves)' % (n, s1.size, s2.size))
                    continue
                at = c['load']
                for i in range(s1.size):
                    if s1.data[i] != s2.data[i]:
                        print('     %s+%d at 0x%06X: 0x%04X, original 0x%04X' % (n, i, at + i, img[at + i - d316.BASE], s2.data[i]))
        for st in new_files: print('   new file: %s' % st)

    # ---- 4. moved, not changed ----------------------------------------------------------------
    res = moved_check(objs, members, orig, lib_orig, order, comps, img, orig_img, changed, set(new_files))
    print('4. moved, not changed: %d sections (%d words) compared with the original\'s words where it has them: '
          '%d words identical, %d relocated fields follow their targets%s' % (
              res['sections'], res['words'], res['same'], res['followed'],
              '' if not res['bad'] else '; %d FAIL' % len(res['bad'])))
    print('5. numbers: %d instructions in those sections hold an address in a floating region (calls, long branches, '
          'MOVL XARn,# into flash); %s' % (res['refs'], 'each of them, and each table pointer to a map address, is a relocated field' if not res['numeric'] else
                               '%d are NOT relocated (written as numbers; they would not follow a move)' % len(res['numeric'])))
    for b in res['numeric'][:12]: print('   FAIL ' + b)
    if res['numeric']: errors.append('addresses written as numbers')
    if res['unchecked']: print('   %d fields differ in expression relocations (not decoded): %s' % (len(res['unchecked']), ', '.join(res['unchecked'][:6])))
    for b in res['bad'][:12]: print('   FAIL ' + b)
    if res['bad']: errors.append('moved check')
    if res['absent']: print('   original sections not in the build: %s' % ', '.join(res['absent'][:12]))

    # ---- summary -------------------------------------------------------------------------------
    print()
    print('| Region | Range | Used | Free |')
    print('|---|---|---|---|')
    for m in x['areas']:
        print('| %s | 0x%06X..0x%06X | %d | %d |' % (m['name'], m['origin'], m['origin'] + m['length'] - 1, m['used'], m['length'] - m['used']))
    print()
    for g in ('.text', 'ramfuncs', '.cinit', '.econst'):
        lo, run, n = groups[g]
        print('  %-9s load 0x%06X..0x%06X  %5d words%s' % (g, lo, lo + n - 1, n, '' if run == lo else '  (run 0x%04X..0x%04X)' % (run, run + n - 1)))
    if errors:
        print('FAIL: %s; no image written  (%.1f s)' % ('; '.join(errors), time.time() - t0))
        d316.stop_wine(); sys.exit(1)
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    shutil.copy(os.path.join(work, 'mod.bin'), a.out)
    base = os.path.splitext(a.out)[0]
    shutil.copy(os.path.join(work, 'mod.map'), base + '.map')       # the linker's map
    shutil.copy(os.path.join(work, 'mod.xml'), base + '.xml')       # its XML (tools/emudiff.py reads it)
    nd = sum(1 for i in range(0x10000) if img[i] != orig_img[i])
    print('wrote %s: 131072 bytes, big-endian 16-bit words 0x3E8000..0x3F7FFF, version %d; %s'
          % (os.path.relpath(a.out, d316.ROOT), ver, '%d words differ from the original' % nd if nd else
             'identical to the original word for word'))
    print('PASS  (%.1f s)' % (time.time() - t0))
    d316.stop_wine()


def read_xml(path):
    r = ET.parse(path).getroot()
    files = {}
    for f in r.find('input_file_list'):
        kind, name = f.findtext('kind'), f.findtext('name')
        files[f.get('id')] = ('rts:' + name) if kind == 'archive' else os.path.splitext(name)[0]
    h = lambda e, t: int(e.findtext(t), 16)
    comps, byid = [], {}
    for c in r.find('object_component_list'):
        if c.find('input_file_ref') is None: continue
        name = c.findtext('name')
        load = c.findtext('load_address') or c.findtext('run_address')
        if load is None: continue
        run = c.findtext('run_address') or load
        d = dict(name=name, key=(files[c.find('input_file_ref').get('idref')], name), load=int(load, 16),
                 run=int(run, 16), size=h(c, 'size'))
        comps.append(d); byid[c.get('id')] = d
    groups = {}
    for g in r.find('logical_group_list'):
        if g.find('load_address') is None: continue
        groups[g.findtext('name')] = (h(g, 'load_address'), h(g, 'run_address'), h(g, 'size'))
    syms = {s.findtext('name'): int(s.findtext('value'), 16) for s in r.find('symbol_table')}
    areas = []
    for m in r.find('placement_map'):
        areas.append(dict(name=m.findtext('name'), page=h(m, 'page_id'), origin=h(m, 'origin'), length=h(m, 'length'),
                          used=h(m, 'used_space')))
    return dict(comps=comps, groups=groups, syms=syms, areas=areas)


def field(typ, ws, k):
    """the value a relocated field holds in words ws at k"""
    if typ == 'R_REL22': return ((ws[k] & 0x3F) << 16) | ws[k + 1]
    if typ in ('R_RELLONG', 'R_ANKPTR'): return ws[k] | ws[k + 1] << 16
    if typ == 'R_PARTLS6': return ws[k] & 0x3F
    return ws[k]


def moved_check(objs, members, orig, lib_orig, order, comps, img, orig_img, changed, new_files):
    """Each section the mod did not change, compared with the original image where the original
    has it: a word may differ only inside a relocated field, and the field must hold the
    original's target translated by the move of the section that holds that target."""
    # every section's original and new addresses, load and run
    where = {}
    for c in comps: where[c['key']] = c
    old = {}                                    # key -> (load, run, size) in the original
    for (st, name), (load, run) in orig.items():
        if load is None: continue
        sec = objs[st].sections[name]
        old[(st, name)] = (load, run, sec.size)
    for (m, name), addr in lib_orig.items():
        old[('rts:' + m, name)] = (addr, addr, members[m].sections[name].size)
    # .cinit: the original's pieces in the original order
    k = CINIT_LO
    for e in order:
        if e.startswith('rts:'):
            # a member's .cinit subsections (_lock.obj's .cinit:__lock, .cinit:__unlock) in the
            # order the linker takes them, which this link shows
            secs = [(name, sec) for name, sec in members[e[4:]].sections.items() if name.startswith('.cinit') and sec.size]
            secs.sort(key=lambda ns: where[(e, ns[0])]['load'] if (e, ns[0]) in where else 0)
            for name, sec in secs:
                old[(e, name)] = (k, k, sec.size); k += sec.size
        elif e in objs:
            for name, sec in objs[e].sections.items():
                if name.startswith('.cinit') and sec.size:
                    old[(e, name)] = (k, k, sec.size); k += sec.size
    spans = []                                  # (original start, size, new start)
    for key, (load, run, size) in old.items():
        c = where.get(key)
        if not c: continue
        spans.append((load, size, c['load']))
        if run != load: spans.append((run, size, c['run']))
    spans.sort()
    starts = [s_[0] for s_ in spans]

    def translate(a_):
        i = bisect.bisect_right(starts, a_) - 1
        if i >= 0 and spans[i][0] <= a_ < spans[i][0] + spans[i][1]:
            return spans[i][2] + a_ - spans[i][0]
        for j in (i, i - 1):                    # just past the end of a section (an end pointer)
            if j >= 0 and a_ == spans[j][0] + spans[j][1]: return spans[j][2] + spans[j][1]
        return a_                               # fixed: RAM, registers, the boot loader, the ROM

    out = dict(sections=0, words=0, same=0, followed=0, bad=[], unchecked=[], absent=[], refs=0, numeric=[])
    # the original's instructions that hold a 22-bit address (calls, long branches, MOVL XARn,#):
    # one into what floats must be a relocated field, or it would not follow a move
    dis = {}
    for addr, ws, txt in d316.image_dis():
        m = re.match(r'(LCR|LB|LC|FFC\s+XAR7,|MOVL\s+XAR\d,)\s*#?0x([0-9a-f]+)$', txt)
        if m: dis[addr] = (int(m.group(2), 16), txt)
    starts_in_map = {r['addr'] for r in d316.functions() + d316.data() if APP_LO <= r['addr'] < APP_HI} | \
                    {d316.run_addr(r) for r in d316.functions() if RAMFUNC_LO <= d316.run_addr(r) < RAMFUNC_HI}
    for key, (load, run, size) in sorted(old.items(), key=lambda kv: kv[1][0]):
        st, name = key
        if st in changed and any(n.split(' ')[0] == name for n in changed[st]): continue
        c = where.get(key)
        if c is None:
            out['absent'].append('%s(%s)' % key); continue
        if not (d316.BASE <= load < d316.END): continue          # .ebss: no words
        sec = members[st[4:]].sections[name] if st.startswith('rts:') else objs[st].sections[name]
        ow = orig_img[load - d316.BASE:load - d316.BASE + size]
        nw = img[c['load'] - d316.BASE:c['load'] - d316.BASE + size]
        out['sections'] += 1; out['words'] += size
        roffs = {off for off, typ, sym in sec.relocs}
        if sec.kind == 'text' and load < SECTOR_A:
            for i in range(size):
                t = dis.get(load + i)
                # MOVL XARn,#0x8000 can be a number (isqrt_div); only a call or branch counts in RAM
                if t and (APP_LO <= t[0] < APP_HI or RAMFUNC_LO <= t[0] < RAMFUNC_HI and not t[1].startswith('MOVL')):
                    out['refs'] += 1
                    if i not in roffs: out['numeric'].append('%s(%s)+%d %s' % (key + (i, t[1])))
        elif sec.kind == 'data':                # 32-bit pointers in tables: a pair that is a map address
            for i in range(0, size - 1):
                v = ow[i] | ow[i + 1] << 16
                if v in starts_in_map and i not in roffs:
                    out['numeric'].append('%s(%s)+%d .long 0x%X' % (key + (i, v)))
        diff = {i for i in range(size) if ow[i] != nw[i]}
        out['same'] += size - len(diff)
        covered, page = set(), {}
        for off, typ, sym in sorted(sec.relocs):
            if typ in d316.RELOC_NOFIELD: continue
            fl = d316.RELOC_FIELDS.get(typ, [(0, 0xFFFF), (1, 0xFFFF)])
            idx = [off + j for j, _ in fl if off + j < size]
            if not any(i in diff for i in idx):
                if typ == 'R_PARTMS16': page[sym] = (ow[off], nw[off], off)
                continue
            covered.update(idx)
            what = '%s+%d %s %s' % ('%s(%s)' % key, off, typ, sym)
            if typ in ('R_REL22', 'R_RELLONG', 'R_ANKPTR'):
                if field(typ, nw, off) == translate(field(typ, ow, off)): out['followed'] += 1
                else: out['bad'].append('%s: 0x%X, the original\'s 0x%X moves to 0x%X' % (
                    what, field(typ, nw, off), field(typ, ow, off), translate(field(typ, ow, off))))
            elif typ == 'R_PARTMS16':
                page[sym] = (ow[off], nw[off], off)
            elif typ == 'R_PARTLS6' and sym in page:
                op, np_, poff = page[sym]
                if (np_ << 6 | nw[off] & 0x3F) == translate(op << 6 | ow[off] & 0x3F): out['followed'] += 1
                else: out['bad'].append(what + ': DP access to a moved target')
                covered.add(poff)
            elif typ.startswith('RE_'):
                out['unchecked'].append(what)
            else:
                # a 16-bit field holds the low half of an address (c_int00's cinit): try it
                # with the high halves of RAM and of flash
                if any(nw[off] == translate(hi << 16 | ow[off]) & 0xFFFF for hi in (0, 0x3E, 0x3F)): out['followed'] += 1
                else: out['bad'].append('%s: 0x%04X, original 0x%04X' % (what, nw[off], ow[off]))
        # a MOVW DP whose LS6 partner did not differ
        for sym, (op, np_, poff) in page.items():
            if poff in diff and poff not in covered:
                covered.add(poff)
                out['bad'].append('%s(%s)+%d R_PARTMS16 %s: the data page changed alone' % (key + (poff, sym)))
        for i in sorted(diff - covered):
            out['bad'].append('%s(%s)+%d: 0x%04X, original 0x%04X, not a relocated field' % (key + (i, nw[i], ow[i])))
    return out

if __name__ == '__main__':
    main()
