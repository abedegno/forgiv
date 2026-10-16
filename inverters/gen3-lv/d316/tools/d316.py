"""Shared code for the D316 tools: configuration, the image, TI's tools (native and under Wine),
COFF objects read through ofd2000, disassembly with dis2000, and the map.

Nothing here writes outside build/ except what a caller asks for."""
import os as _os, sys as _sys
# tools/dis.py hides the standard library's dis module for every script in tools/: load the real
# one (through inspect) with tools/ off the path, before anything else asks for it
_tools = _os.path.dirname(_os.path.abspath(__file__))
_saved = list(_sys.path)
_sys.path[:] = [p for p in _sys.path if _os.path.abspath(p or '.') != _tools]
import inspect  # noqa: E402,F401
_sys.path[:] = _saved
import contextlib, fcntl, functools, os, re, shutil, socket, struct, subprocess, sys, tempfile, tomllib
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, 'build')


def _x(p):
    return os.path.abspath(os.path.join(ROOT, os.path.expanduser(p)))


# The machine's paths: toolchain.toml holds defaults, toolchain.local.toml (gitignored; start from
# toolchain.example.toml) overrides any key of it, and these environment variables win over both.
ENV = {('compiler', 'cgt'): 'C2000_CGT', ('compiler', 'rtslib'): 'C2000_RTSLIB',
       ('compiler', 'others'): 'C2000_OTHERS', ('native', 'bin'): 'C2000_NATIVE',
       ('wine', 'bin'): 'WINE_BIN', ('wine', 'prefix'): 'C2000_WINEPREFIX'}


def _merge(a, b):
    for k, v in b.items():
        if isinstance(v, dict) and isinstance(a.get(k), dict): _merge(a[k], v)
        else: a[k] = v


@functools.cache
def cfg():
    with open(os.path.join(ROOT, 'toolchain.toml'), 'rb') as f:
        c = tomllib.load(f)
    local = os.path.join(ROOT, 'toolchain.local.toml')
    if os.path.exists(local):
        with open(local, 'rb') as f: _merge(c, tomllib.load(f))
    for (sec, key), var in ENV.items():
        if os.environ.get(var): c[sec][key] = os.environ[var]
    # {prefix} in a path is the Wine prefix, so the defaults follow C2000_WINEPREFIX
    prefix = _x(c['wine']['prefix'])
    for sec, key in ENV:
        if isinstance(c[sec].get(key), str): c[sec][key] = c[sec][key].replace('{prefix}', prefix)
    c['wine']['prefix'] = prefix
    c['compiler']['cgt'] = _x(c['compiler']['cgt'])
    c['compiler']['rtslib'] = os.path.expanduser(c['compiler']['rtslib'])   # or relative to cgt
    c['compiler']['others'] = _x(c['compiler']['others'])
    c['native']['bin'] = _x(c['native']['bin'])
    if not os.path.isdir(c['wine']['bin']):
        w = shutil.which('wine')
        if w: c['wine']['bin'] = os.path.dirname(w)
    return c


BASE = 0x3E8000
END = BASE + 0x10000


# ---- the sources -------------------------------------------------------------------------------
SRC = os.path.join(ROOT, 'src')
INC = os.path.join(SRC, 'include')


def sources(exts=('.c', '.asm')):
    """every source file under src/ (in subdirectories too, not src/include), sorted by path.
    The tools name an object by its file's stem, so stems must be unique: ToolError if not."""
    out = []
    for d, dirs, files in os.walk(SRC):
        if os.path.commonpath([d, INC]) == INC: dirs[:] = []; continue
        dirs.sort()
        out += [os.path.join(d, n) for n in sorted(files) if n.endswith(exts)]
    seen = {}
    for p in out:
        s = os.path.splitext(os.path.basename(p))[0]
        if s in seen:
            raise ToolError('two sources with the stem %s: %s and %s'
                            % (s, os.path.relpath(seen[s], ROOT), os.path.relpath(p, ROOT)))
        seen[s] = p
    return out


def source(name):
    """the source under src/ with this file name or stem (e.g. 'grid_support.c' or 'grid_support')"""
    for p in sources():
        b = os.path.basename(p)
        if name in (b, os.path.splitext(b)[0]): return p
    raise ToolError('no source %s under src/' % name)


# ---- the image ---------------------------------------------------------------------------------
@functools.cache
def image():
    c = cfg()['image']
    b = open(os.path.join(ROOT, c['bin']), 'rb').read()
    assert len(b) == 2 * c['words'], 'image size'
    return struct.unpack('>%dH' % (len(b) // 2), b)


def W(a):
    return image()[a - BASE]


def words(a, n):
    return list(image()[a - BASE:a - BASE + n])


# ---- native tools ------------------------------------------------------------------------------
def native(tool):
    return os.path.join(cfg()['native']['bin'], tool)


def run(args, cwd=None, check=True, env=None, timeout=300):
    # a Wine tool now and then hangs (seen once in lnk2000): after `timeout` seconds stop that
    # prefix's wineserver, which ends the hung process, and try once more
    for attempt in (1, 2):
        try:
            p = subprocess.run(args, cwd=cwd, capture_output=True, text=True, errors='replace', env=env,
                               stdin=subprocess.DEVNULL, timeout=timeout)
            break
        except subprocess.TimeoutExpired:
            prefix = (env or {}).get('WINEPREFIX')
            if prefix:
                subprocess.run([os.path.join(cfg()['wine']['bin'], 'wineserver'), '-k'],
                               env=dict(os.environ, WINEPREFIX=prefix), capture_output=True)
            if attempt == 2:
                raise ToolError(f'{os.path.basename(args[0])} hung for {timeout} s twice')
    if check and p.returncode:
        raise ToolError(f'{os.path.basename(args[0])} failed ({p.returncode}):\n{p.stdout}{p.stderr}')
    return p


class ToolError(Exception):
    pass


def scratch(prefix='d316'):
    os.makedirs(os.path.join(BUILD, 'tmp'), exist_ok=True)
    return tempfile.TemporaryDirectory(prefix=prefix + '-', dir=os.path.join(BUILD, 'tmp'))


# ---- Wine --------------------------------------------------------------------------------------
# A Wine prefix has one wineserver. Every tool process takes a job prefix of its own: an APFS clone
# (cp -Rc) of the master prefix under build/wine/jobN, held with an flock while in use, so tools
# can run side by side without sharing a server.
WINE_JOBS = 8


def _server_alive(prefix):
    st = os.stat(prefix)
    sock = f'/tmp/.wine-{os.getuid()}/server-{st.st_dev:x}-{st.st_ino:x}/socket'
    if not os.path.exists(sock): return False
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        s.connect(sock); return True
    except OSError:
        return False
    finally:
        s.close()


class Wine:
    """with Wine() as w: w.run([exe, args...], cwd)"""

    def __init__(self, job=None):
        self.job, self.lock = job, None

    def __enter__(self):
        c = cfg()['wine']
        d = os.path.join(BUILD, 'wine')
        os.makedirs(d, exist_ok=True)
        jobs = [self.job] if self.job is not None else range(WINE_JOBS)
        while self.lock is None:
            for n in jobs:
                f = open(os.path.join(d, f'job{n}.lock'), 'w')
                try:
                    fcntl.flock(f, fcntl.LOCK_EX | (0 if self.job is not None else fcntl.LOCK_NB))
                except BlockingIOError:
                    f.close(); continue
                self.lock, self.n = f, n
                break
            else:
                f = open(os.path.join(d, 'job0.lock'), 'w'); fcntl.flock(f, fcntl.LOCK_EX)
                self.lock, self.n = f, 0
        self.prefix = os.path.join(d, f'job{self.n}')
        if not os.path.exists(os.path.join(self.prefix, 'system.reg')):
            shutil.rmtree(self.prefix, ignore_errors=True)
            # an APFS clone on macOS; elsewhere (Linux, CI) a plain copy, or a reflink where the
            # file system has them
            cp = ['cp', '-Rc'] if sys.platform == 'darwin' else ['cp', '-a', '--reflink=auto']
            subprocess.run(cp + [c['prefix'], self.prefix], check=True)
        self.env = dict(os.environ, WINEPREFIX=self.prefix, WINEDEBUG=c.get('debug', '-all'),
                        WINEDLLOVERRIDES='mscoree,mshtml=')
        self.env['PATH'] = c['bin'] + os.pathsep + self.env.get('PATH', '')
        self.wine = os.path.join(c['bin'], 'wine')
        if not _server_alive(self.prefix):
            # a server that stays 60 s after its last client, started with no pipes; then the first
            # program (Wine's services start with it and keep its pipes) also with no pipes
            subprocess.run([os.path.join(c['bin'], 'wineserver'), '-p60'], env=self.env, start_new_session=True,
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            subprocess.run([self.wine, 'cmd', '/c', 'exit'], env=self.env, start_new_session=True,
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300)
        return self

    def run(self, args, cwd=None, check=True):
        return run([self.wine] + list(args), cwd=cwd, check=check, env=self.env)

    def __exit__(self, *a):
        self.lock.close()


def stop_wine():
    c = cfg()['wine']
    d = os.path.join(BUILD, 'wine')
    if not os.path.isdir(d): return
    for n in os.listdir(d):
        p = os.path.join(d, n)
        if os.path.isdir(p) and _server_alive(p):
            subprocess.run([os.path.join(c['bin'], 'wineserver'), '-k'], env=dict(os.environ, WINEPREFIX=p),
                           capture_output=True)


def cgt_bin(tool, cgt=None):
    return os.path.join(cgt or cfg()['compiler']['cgt'], 'bin', tool + '.exe')


def cgt_of(src):
    """The compiler home a source asks for with a line `// cgt: 16.9.6.LTS` (a version, matched
    against the installed compilers of [compiler] others, or a path); None for the default."""
    want = None
    for line in open(src, encoding='latin1'):
        m = re.match(r'\s*//\s*cgt:\s*(\S+)', line)
        if m: want = m.group(1); break
    if want is None: return None
    if os.path.isdir(os.path.expanduser(want)): return os.path.expanduser(want)
    import glob
    for exe in sorted(glob.glob(os.path.expanduser(cfg()['compiler']['others']))):
        home = os.path.dirname(os.path.dirname(exe))
        if os.path.basename(home).endswith('_' + want) or os.path.basename(home) == want:
            return home
    raise ToolError(f'{src}: compiler {want} is not installed')


def compile_c(wine, src, outdir, cflags=None, cgt=None, keep_asm=False, extra=()):
    """Compile src with cl2000 under Wine into outdir; returns the object path. Without cgt, a
    `// cgt: VERSION` line in src picks another installed compiler (cgt_of)."""
    c = cfg()['compiler']
    if cgt is None: cgt = cgt_of(src)
    flags = list(cflags if cflags is not None else c['cflags']) + list(c.get('tool_cflags', [])) + list(extra)
    src, outdir = os.path.abspath(src), os.path.abspath(outdir)
    obj = os.path.join(outdir, os.path.splitext(os.path.basename(src))[0] + '.obj')
    args = [cgt_bin('cl2000', cgt)] + flags + ['-c', '--obj_directory=' + outdir, '--asm_directory=' + outdir,
                                               '-I' + os.path.join(ROOT, 'src', 'include'),
                                               '-I' + os.path.join(cgt or c['cgt'], 'include')]
    if keep_asm: args.append('-k')
    p = wine.run(args + [src], cwd=outdir, check=False)
    if p.returncode or not os.path.exists(obj):
        raise ToolError(f'cl2000 failed on {src}:\n{p.stdout}{p.stderr}')
    return obj


# ---- COFF objects through ofd2000 ---------------------------------------------------------------
class Section:
    def __init__(self, name, size, data, relocs, kind, page, paddr, vaddr):
        self.name, self.size, self.data, self.relocs = name, size, data, relocs
        self.kind, self.page, self.paddr, self.vaddr = kind, page, paddr, vaddr


class Obj:
    def __init__(self, path):
        self.path = path
        x = run([native('ofd2000'), '-x', '--obj_display=none,sections,relocs,symbols,header', path]).stdout
        root = ET.fromstring(x.encode('latin1', 'replace'))
        raw = open(path, 'rb').read()
        self.sections, self.symbols = {}, []
        coff = root.find('.//ti_coff')
        for s in coff.findall('section'):
            name = s.findtext('name')
            size = int(s.findtext('raw_data_size'), 16)
            ptr = int(s.findtext('file_offsets/raw_data_ptr'), 16)
            kind = 'text' if s.find('text') is not None else 'bss' if s.find('bss') is not None else \
                'data' if s.find('data') is not None else 'other'
            if s.find('copy') is not None: kind = 'copy'
            nw = size // 2 if kind != 'bss' else 0
            data = list(struct.unpack('<%dH' % nw, raw[ptr:ptr + 2 * nw])) if nw and ptr else []
            if kind == 'bss': nw = size // 2
            relocs = []
            for r in s.iter('reloc_entry'):
                relocs.append((int(r.findtext('virtual_addr'), 16), r.findtext('type'),
                               r.findtext('symbol_relative') or ('(internal)' if r.find('internal') is not None else None)))
            pg = s.findtext('page')
            self.sections[name] = Section(name, nw if kind == 'bss' else len(data), data, relocs, kind,
                                          int(pg, 16) if pg else 0,
                                          int(s.findtext('physical_addr'), 16), int(s.findtext('virtual_addr'), 16))
        for y in coff.findall('symbol_table/symbol'):
            self.symbols.append(dict(name=y.findtext('name'), kind=y.findtext('kind'), section=y.findtext('section'),
                                     sclass=y.findtext('storage_class'), value=int(y.findtext('value') or '0', 16)))

    def defined(self):
        """global symbols defined here: name -> (section, offset)"""
        return {s['name']: (s['section'], s['value']) for s in self.symbols
                if s['sclass'] == 'C_EXT' and s['kind'] == 'defined'}

    def undefined(self):
        return sorted({s['name'] for s in self.symbols if s['sclass'] == 'C_EXT' and s['kind'] == 'undefined'})


# the bits of each word a relocation type writes, as (word offset from the relocation, mask)
RELOC_FIELDS = {
    'R_REL22': [(0, 0x003F), (1, 0xFFFF)],      # LCR/LB/MOVL XARn,#22: 6 bits in the opcode, 16 in the next word
    'R_PARTLS6': [(0, 0x003F)],                 # @6-bit direct offset in the opcode
    'R_PARTMS16': [(0, 0xFFFF)],                # MOVW DP,#16: the data page
    'R_RELWORD': [(0, 0xFFFF)],
    'R_ANKHI16': [(0, 0xFFFF)],
    'R_ABS16_OC': [(0, 0xFFFF)],
    'RE_XSTFLD': [(0, 0xFFFF)],                 # the store at the end of an expression relocation
    'R_ANKPTR': [(0, 0xFFFF), (1, 0xFFFF)],     # .cinit and pointer data: 32 bits
    'R_RELLONG': [(0, 0xFFFF), (1, 0xFFFF)],
}
RELOC_NOFIELD = ('RE_PUSH', 'RE_PUSHUK', 'RE_ASR', 'RE_ADD', 'RE_SUB', 'RE_AND', 'RE_OR', 'RE_SR',
                 'RE_SL', 'RE_PUSHSK', 'RM_OBJ', 'RM_RANGE', 'RM_DWARF_LENGTH')


def reloc_masks(sec):
    """per word of sec, the bits relocations write (0 for a word no relocation touches)"""
    m = [0] * len(sec.data)
    for off, typ, sym in sec.relocs:
        if typ in RELOC_NOFIELD: continue
        for k, bits in RELOC_FIELDS.get(typ, [(0, 0xFFFF), (1, 0xFFFF)]):
            if 0 <= off + k < len(m): m[off + k] |= bits
    return m


def derive_bases(sec, load):
    """Where the image puts the targets of sec's relocations, for sec's words at load address
    `load` in the image: {symbol: [address of the symbol's offset 0, ...]} (one entry per use).
    R_REL22 gives the address at once; a DP access gives it from the MOVW DP page (R_PARTMS16)
    and the 6-bit offset (R_PARTLS6) that follows it for the same symbol."""
    out = {}
    page = {}
    for off, typ, sym in sorted(sec.relocs):
        if not sym or typ in RELOC_NOFIELD: continue
        a = load + off
        if not BASE <= a < END - 1: continue
        if typ == 'R_REL22':
            img = ((W(a) & 0x3F) << 16) | W(a + 1)
            obj = ((sec.data[off] & 0x3F) << 16) | sec.data[off + 1]
            out.setdefault(sym, []).append(img - obj)
        elif typ == 'R_PARTMS16':
            page[sym] = (W(a), sec.data[off])
        elif typ == 'R_PARTLS6' and sym in page:
            ip, op = page[sym]
            out.setdefault(sym, []).append((ip << 6 | W(a) & 0x3F) - (op << 6 | sec.data[off] & 0x3F))
    return out


def out_load_words(path):
    """A linked .out: {load address: words} for every initialised section, and the section list."""
    o = Obj(path)
    segs = {}
    for s in o.sections.values():
        if s.kind in ('text', 'data') and s.data:
            segs[s.paddr] = (s.name, s.data)
    return segs, o


# ---- disassembly -------------------------------------------------------------------------------
_DIS = re.compile(r'^([0-9a-f]{8})\s+([0-9a-f]{4})\s+(.*)$')
_CONT = re.compile(r'^([0-9a-f]{8})\s+([0-9a-f]{4})\s*$')


def disasm(ws, addr, cache_key=None):
    """dis2000's reading of words ws placed at word address addr: [(addr, [words], text)]."""
    if cache_key:
        p = os.path.join(BUILD, 'dis', cache_key + '.dis')
        if not os.path.exists(p):
            os.makedirs(os.path.dirname(p), exist_ok=True)
            txt = _dis_text(ws, addr)
            open(p + '.tmp', 'w').write(txt); os.replace(p + '.tmp', p)
        txt = open(p).read()
    else:
        txt = _dis_text(ws, addr)
    out = []
    for line in txt.splitlines():
        m = _DIS.match(line)
        if m and m.group(3).strip() and not m.group(3).startswith('  '):
            out.append([int(m.group(1), 16), [int(m.group(2), 16)], ' '.join(m.group(3).split())])
        elif m and out and int(m.group(1), 16) == out[-1][0] + len(out[-1][1]) and not m.group(3).strip():
            out[-1][1].append(int(m.group(2), 16))
        elif out and line.strip().startswith('||') and not _DIS.match(line):
            # the second half of a parallel FPU instruction (MPYF32 ... || MOV32 ...)
            out[-1][2] += ' ' + ' '.join(line.split())
        else:
            m = _CONT.match(line)
            if m and out and int(m.group(1), 16) == out[-1][0] + len(out[-1][1]):
                out[-1][1].append(int(m.group(2), 16))
    return [tuple(x) for x in out]


def _dis_text(ws, addr):
    with scratch('dis') as d:
        with open(os.path.join(d, 'w.asm'), 'w') as f:
            f.write('\t.sect "w"\n')
            for i in range(0, len(ws), 16):
                f.write('\t.word ' + ','.join('0x%04X' % x for x in ws[i:i + 16]) + '\n')
        with open(os.path.join(d, 'w.cmd'), 'w') as f:
            f.write('MEMORY { PAGE 0: M : origin = 0x%X, length = 0x%X }\n' % (addr, max(len(ws), 1)))
            f.write('SECTIONS { w : > M, PAGE 0 }\n')
        run([native('asm2000'), '-v28', '--float_support=fpu32', 'w.asm'], cwd=d)
        run([native('lnk2000'), '-a', '-o', 'w.out', 'w.obj', 'w.cmd'], cwd=d)
        return run([native('dis2000'), '-i', 'w.out'], cwd=d).stdout


def image_dis():
    """dis2000 over the whole image (cached in build/dis)."""
    return disasm(image(), BASE, cache_key='image')


# ---- the map -----------------------------------------------------------------------------------
MAPDIR = os.path.join(ROOT, 'map')
FCOLS = ['addr', 'size', 'name', 'kind', 'notes']
DCOLS = ['addr', 'size', 'name', 'kind', 'notes']


def read_tsv(path):
    rows = []
    if not os.path.exists(path): return rows
    for line in open(path):
        if line.startswith('#') or not line.strip(): continue
        f = line.rstrip('\n').split('\t')
        if f[0] == 'addr': continue
        f += [''] * (5 - len(f))
        rows.append(dict(addr=int(f[0], 16), size=int(f[1]) if f[1] else 0, name=f[2], kind=f[3], notes=f[4]))
    return rows


def write_tsv(path, rows, header):
    with open(path + '.tmp', 'w') as f:
        f.write('\t'.join(header) + '\n')
        for r in rows:
            f.write('%06X\t%s\t%s\t%s\t%s\n' % (r['addr'], r['size'] if r['size'] else '', r['name'], r['kind'], r['notes']))
    os.replace(path + '.tmp', path)


def functions():
    return read_tsv(os.path.join(MAPDIR, 'functions.tsv'))


def data():
    return read_tsv(os.path.join(MAPDIR, 'data.tsv'))


def note(row, key):
    """key=value from a row's notes (e.g. run=0x8000)"""
    m = re.search(r'(?:^|[ ;])' + re.escape(key) + r'=(\S+)', row['notes'])
    return m.group(1).rstrip(';,') if m else None


def run_addr(row):
    """the address a function runs at (ramfuncs run in RAM)"""
    r = note(row, 'run')
    return int(r, 16) if r else row['addr']


_HEXNAME = re.compile(r'^(?:sub|fn|d|var|tbl|DAT|FUN|sw|ram|c)_([0-9A-Fa-f]{4,6})$')


@functools.cache
def symtab():
    """C-level name (no leading _) -> the address code refers to it by (run address for ramfuncs)."""
    t = {}
    for r in functions():
        t.setdefault(r['name'], run_addr(r))
        for n in (note(r, 'alias') or '').split(','):     # other names of a library entry point
            if n: t.setdefault(n[1:] if n.startswith('_') else n, run_addr(r))
    for r in data():
        t.setdefault(r['name'], r['addr'])
    return t


def resolve(sym):
    """A COFF symbol (C names have a leading _) -> address, or None. Names in the map resolve to
    their rows; names like sub_3EAC5D or d_D8E2 (or 0xD8E2) to that address."""
    n = sym[1:] if sym.startswith('_') else sym
    t = symtab()
    if n in t: return t[n]
    m = _HEXNAME.match(n)
    if m: return int(m.group(1), 16)
    if re.match(r'^0x[0-9A-Fa-f]+$', n): return int(n, 16)
    return None


def fmt_words(ws):
    return ' '.join('%04x' % w for w in ws)
