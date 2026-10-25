"""The A319 gate: prove every proved source, then rebuild the whole image by linking and compare
it with the original, byte for byte, as an image and as the shipped ARMStore.bin.

tools/a319env.py finds Exhume, the MDK homes and Wine (EXHUME, MDK_HOME, MDK_LIBHOME, WINE_BIN,
or toolchain.local.toml); the images are the ones tools/fetch_firmware.py at the repository root
puts in firmware/gen3-lv/.

    $EXHUME/.venv/bin/python tools/gate.py [--fast] [--jobs N] [--verify-only] [--libhome toolchain/MDKxxx]
        [--place STEM,...|all] [--with SRC ...]
    $EXHUME/.venv/bin/python tools/gate.py --whole [--fast] [--jobs N] [--with SRC ...]

Three stages (docs/gate.md has the why):

1. Verify. Every source under src/ (not src/include) is a proved source: Exhume's armmatch
   compiles it with its own /* opts: */ line, places each section at the address its at: lines
   and the map give, links it with symdefs for everything else and compares every byte with the
   image (RAM data with the image's RW image). A source that is not a whole match fails the gate.
2. The library link. The standard C library's startup (__main, __scatterload, the region
   handlers, __rt_entry and friends), the library members after the application code, the
   region table and the compressed RW data are linked from the toolchain's own libraries, laid
   out by armlink as the original link did: a stub asks for the members the application calls,
   the RW data is the image's own initial RAM, the zero-init data a blob around the library's
   __libspace. Compared over its three ranges.
3. Pieces. MDK-Lite links at most 32 KB of RO+RW, so the rest of the flash image is linked in
   pieces. Each piece places the proved sources' sections that fall in it FIXED at their
   addresses and fills every byte that no proved source and no library range covers with the
   original's bytes (an assembler INCBIN blob per gap); every other symbol comes from symdefs.

The rebuilt image is the union of the piece links and the library link; it must be identical to
the image of A319_ARMStore.bin, and with the 4-byte header (u16 version, u16 0x5566)
identical to that file. A report says how many bytes each kind of source supplied.

A source too large for MDK-Lite's armlink (L6050U: more than 32 KB of RO and RW) is proved by
object placement instead (tools/objplace.py): its object's sections are placed at their addresses
and every relocation resolved by a small linker, and every byte compared, unmasked. Such a unit is
left out of the pieces; its bytes in the image come from that placement. --place STEM,...|all
(or GATE_PLACE) proves the named sources that way too, armlink or not.

--fast reuses what an earlier run proved and linked when its inputs are unchanged (the source
and the headers it includes, the map, the toolchain): only changed sources are verified again and
only the links whose inputs changed run again. Without --fast everything is rebuilt.

--whole needs a licensed MDK (MDK_LICENSED_HOME, or mdk_licensed_home in toolchain.local.toml;
unset, it says so and exits 0). Every source is verified with armmatch and that MDK's armlink, so
no source is proved by object placement, and then one armlink run links the whole image: every
proved section FIXED at its address (in the original's input order), the library's start-up code
and members from the library home, the region table after the last .constdata where armlink
aligns it, and one RW region made of the image's initial RAM around the proved RAM sections
(stm32f10x_rcc's .data), the library's .bss and the startup file's HEAP and STACK, compressed by
armlink. The listing must show every section at its address and nothing between sections but
armlink's alignment padding; the image, bin and hex must be identical. The two C stand-ins for
library members (src/clib) are left out of that link and compared with the members. Its output
is build/whole (report.txt, A319.img, A319_ARMStore.bin, A319.hex, the link in build/whole/link).

Wine: at most --jobs (default 4) worker processes, each with its own prefix under
build/gate/wine/jobN and a persistent wineserver, stopped at the end."""
import hashlib, json, os, pickle, re, shutil, struct, subprocess, sys, time
from dataclasses import dataclass, field

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.environ.setdefault('EXHUME_ARMCC_RUNNER', 'wine')      # the gate's workers use Wine prefixes of their own
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import a319env  # noqa: E402  (finds Exhume, the MDK homes and Wine)
EXHUME = a319env.EXHUME
# the vendored rwdata (the standard library's region table, from Exhume's armcc5-stdlib branch)
# goes first, so armmatch and everything else import it rather than whatever Exhume has checked out
sys.path[:0] = [os.path.join(ROOT, 'tools', 'vendor'), os.path.join(EXHUME, 'tools'),
                os.path.join(EXHUME, 'profiles', 'armcc5', 'tools')]
import config, armtc, armmap, armmatch, ehex, rwdata          # noqa: E402
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import objplace                                                 # noqa: E402

_exhume_place = armmatch.place


def place_weak(info, m, extra, unused_ok=False):
    """armmatch.place, with a weak symbol named on an at: line at another address taken as
    overridden there, as armmatch already takes one the map names elsewhere. The startup file's
    default handlers are weak; the application's real handlers are map rows with placeholder
    names, so the startup source names them with at: lines (`; at: NMI_Handler 0x0801432C`).
    Such a name neither places its section nor counts as defined by the source, so the symdefs
    define it at the real handler's address."""
    weak = {s.name for s in info.symbols if s.bind == 'Wk'}
    over = {n for n in extra if n in weak}
    placements, defined, errors, unplaced = _exhume_place(info, m, {k: v for k, v in extra.items() if k not in over}, unused_ok)
    for n in over:
        if defined.get(n) != extra[n]: defined.pop(n, None)
    return placements, defined, errors, unplaced


armmatch.place = place_weak          # armmatch.match() and place_all() both go through it

VERSION = 4                       # bump when a change here should invalidate --fast caches
# the gate's output (--whole writes build/whole; its workers learn it from GATE_DIR)
GATE = os.environ.get('GATE_DIR') or os.path.join(ROOT, 'build', 'gate')
WINE = os.path.join(ROOT, 'build', 'gate', 'wine')     # the workers' Wine prefixes, shared by every mode
LITE = 0x8000                     # MDK-Lite: RO + RW of one link
PIECE = 0x7000                    # nominal piece size, below LITE with room for other sections
TIMEOUT = 1800                    # seconds for one stage's worker results: a hung Wine fails the gate
WORK_ASIDE = 0x0E000000           # load addresses of pad regions (nothing compares them)


# ---- configuration -----------------------------------------------------------------------------
def gate_cfg(cfg):
    g = cfg.raw.get('gate', {})
    h = lambda v: int(str(v), 16)
    return dict(
        bin=os.path.join(cfg.root, g.get('bin', '../../../firmware/gen3-lv/A319_ARMStore.bin')),
        version=int(g.get('version', 0)), magic=h(g.get('magic', '5566')),
        libhome=os.path.join(cfg.root, g.get('libhome', 'toolchain/MDK516')),
        lib_ranges=[(h(a), h(b)) for a, b in g.get('lib_ranges', [])],
        lib_requests=list(g.get('lib_requests', [])),
        lib_symdefs={k: h(v) for k, v in g.get('lib_symdefs', {}).items()},
        libspace=h(g.get('libspace', '0')), heap=h(g.get('heap', '0')),
        jobs=int(os.environ.get('GATE_JOBS') or g.get('jobs', 4)))


def sources(cfg, extra=()):
    """Every proved source; extra paths (--with, for tests) are added, each in place of the src/
    source with the same stem."""
    out = []
    inc = os.path.join(cfg.root, 'src', 'include')
    for d, dirs, files in os.walk(os.path.join(cfg.root, 'src')):
        if os.path.commonpath([d, inc]) == inc: dirs[:] = []; continue
        dirs.sort()
        for n in sorted(files):
            if n.endswith(('.c', '.s')): out.append(os.path.join(d, n))
    for x in extra:
        out = [p for p in out if stem(p) != stem(x)] + [os.path.abspath(x)]
    seen = {}
    for p in out:
        s = stem(p)
        if s in seen: raise SystemExit(f'two sources with the stem {s}: {seen[s]} and {p}')
        seen[s] = p
    return out


def stem(p):
    return os.path.splitext(os.path.basename(p))[0]


def rel(p):
    return os.path.relpath(p, ROOT)


def sha(*parts):
    h = hashlib.sha1()
    for p in parts: h.update(p if isinstance(p, bytes) else str(p).encode()); h.update(b'\0')
    return h.hexdigest()


_INC = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)


def includes(path, opts, seen=None):
    """Every file a source includes with "..." (relative to it, then its -I directories), recursively."""
    seen = set() if seen is None else seen
    dirs = re.findall(r'-I\s*(\S+)', opts or '')
    try:
        text = open(path, encoding='latin1').read()
    except OSError:
        return seen
    for name in _INC.findall(text):
        for d in [os.path.dirname(path)] + dirs:
            p = os.path.normpath(os.path.join(d, name))
            if os.path.isfile(p):
                if p not in seen:
                    seen.add(p); includes(p, opts, seen)
                break
    return seen


def source_key(cfg, src, mapkey, mode=''):
    _, opts = cfg.directives(src) if src.endswith('.c') else (None, '')
    parts = [VERSION, mapkey, cfg.home, mode, open(src, 'rb').read()]
    for p in sorted(includes(src, opts)): parts += [p, open(p, 'rb').read()]
    return sha(*parts)


# ---- workers -----------------------------------------------------------------------------------
_W = {}


def wine_env(n):
    prefix = os.path.join(WINE, f'job{n}')
    os.environ['WINEPREFIX'] = prefix
    os.environ.setdefault('WINEDLLOVERRIDES', 'mscoree,mshtml=')
    os.environ.setdefault('WINEDEBUG', '-all')
    return prefix


def wlog(msg):
    with open(os.path.join(WINE, f'job{_W.get("n", "x")}.log'), 'a') as f: f.write(f'{time.strftime("%X")} {os.getpid()} {msg}\n')


def _init(queue):
    n = queue.get()
    _W['n'] = n
    prefix = wine_env(n)
    os.makedirs(WINE, exist_ok=True)     # a fresh checkout has no build/
    wlog('init')
    import faulthandler, signal
    _W['tb'] = open(os.path.join(WINE, f'job{n}.log'), 'a')
    faulthandler.register(signal.SIGUSR1, file=_W['tb'])
    wine = shutil.which('wine')
    bindir = os.path.dirname(wine) if wine else ''
    if not os.path.exists(os.path.join(prefix, 'system.reg')):
        os.makedirs(prefix, exist_ok=True)
        subprocess.run([os.path.join(bindir, 'wineboot'), '-i'], env=os.environ, capture_output=True, timeout=900)
    # a persistent server, started and ready before the first tool runs: a tool that finds no server
    # starts one itself, and that server keeps the tool's output pipe open, so the call never returns
    subprocess.run([os.path.join(bindir, 'wineserver'), '-p'], env=os.environ, start_new_session=True,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=120)
    # the first program in a new server starts Wine's services (services.exe, winedevice ...), which
    # live as long as the server and inherit that program's output pipes: start them here, with no
    # pipes, or the first tool call's output never reaches EOF and the call never returns
    subprocess.run([wine, 'cmd', '/c', 'exit'], env=os.environ, start_new_session=True,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300)
    _W['cfg'] = load_cfg()
    _W['tc'] = armtc.toolchain(_W['cfg'])
    wlog('ready')


def load_cfg():
    cfg = a319env.load()
    cfg.build = os.path.join(GATE, 'verify')          # armmatch's work directories, the gate's own
    return cfg


def stop_wine(jobs):
    wine = shutil.which('wine')
    server = os.path.join(os.path.dirname(wine), 'wineserver') if wine else 'wineserver'
    for n in range(jobs):
        prefix = os.path.join(WINE, f'job{n}')
        if os.path.isdir(prefix):
            subprocess.run([server, '-k'], env=dict(os.environ, WINEPREFIX=prefix), capture_output=True)


class LazyPool:
    """The worker pool, started (and Wine with it) only when there is work: a --fast run with
    nothing changed runs no Wine at all."""
    def __init__(self, jobs):
        self.jobs, self.pool = jobs, None

    def map_async(self, fn, items, chunksize=1):
        if self.pool is None:
            stop_wine(4)          # a server left by an interrupted run can hang new clients
            self.pool = make_pool(self.jobs)
        return self.pool.map_async(fn, items, chunksize=chunksize)

    def close(self):
        if self.pool is not None:
            self.pool.close(); self.pool.join(); stop_wine(self.jobs)


def make_pool(jobs):
    import multiprocessing as mp
    ctx = mp.get_context('spawn')
    manager = ctx.Manager(); queue = manager.Queue()
    for n in range(jobs): queue.put(n)
    p = ctx.Pool(jobs, initializer=_init, initargs=(queue,))
    p._gate_manager = manager
    return p


# ---- stage 1: verify ---------------------------------------------------------------------------
def task_verify(job):
    """One source: (whole, lines, ObjectInfo, object path, seconds, objplace.Obj or None).

    Exhume's armmatch, unless the source is to be placed (mode 'place') or armlink refuses it as
    too large for MDK-Lite (L6050U): then object placement (tools/objplace.py) on the same
    object, and the Obj comes back for the image."""
    src, mode = job
    cfg, tc = _W['cfg'], _W['tc']
    t = time.time()
    try:
        if mode == 'place':
            res, info, obj = objplace.placed_match(cfg, src)
            return res.whole, res.lines, info, obj, time.time() - t, res.obj
        res = armmatch.match(cfg, src)
        obj = os.path.join(cfg.build, stem(src), stem(src) + '.o')
        if any('L6050U' in l for l in res.lines) and os.environ.get('GATE_WHOLE'):
            return False, res.lines + ['-- L6050U under --whole: the licensed MDK did not take the link'], None, None, time.time() - t, None
        if any('L6050U' in l for l in res.lines):
            lines = [l for l in res.lines if 'L6050U' in l] + ['-- too large for armlink: proved by object placement']
            res2, info, obj = objplace.placed_match(cfg, src, compile=False)
            return res2.whole, lines + res2.lines, info, obj, time.time() - t, res2.obj
    except Exception as e:                       # report it with the source rather than lose the gate
        return False, [f'{type(e).__name__}: {e}'], None, None, time.time() - t, None
    info = tc.symbols(obj) if os.path.exists(obj) and 'compile failed' not in ' '.join(res.lines) else None
    return res.whole, res.lines, info, obj, time.time() - t, None


@dataclass
class Proved:
    src: str
    obj: str
    info: object
    whole: bool
    lines: list
    key: str
    placements: list = field(default_factory=list)       # (section, addr, size)
    defined: dict = field(default_factory=dict)
    cached: bool = False
    pobj: object = None          # objplace.Obj: proved by object placement, left out of the pieces
    unplaced: list = field(default_factory=list)


def place_mode(s, place):
    """'place' when the source is to be proved by object placement whatever its size."""
    return 'place' if place == {'all'} or stem(s) in place else ''


def verify_all(cfg, srcs, pool, fast, mapkey, log, place=frozenset()):
    cdir = os.path.join(GATE, 'cache', 'src'); os.makedirs(cdir, exist_ok=True)
    out, todo = {}, []
    for s in srcs:
        key = source_key(cfg, s, mapkey, place_mode(s, place))
        cp = os.path.join(cdir, stem(s) + '.pkl')
        if fast and os.path.exists(cp):
            with open(cp, 'rb') as f: c = pickle.load(f)
            if c['key'] == key and c['obj'] and os.path.exists(c['obj']) and sha(open(c['obj'], 'rb').read()) == c['objsha']:
                out[s] = Proved(s, c['obj'], c['info'], c['whole'], c['lines'], key, cached=True, pobj=c.get('pobj')); continue
        todo.append((s, key))
    t = time.time()
    jobs = [(s, place_mode(s, place)) for s, _ in todo]
    for (s, key), (whole, lines, info, obj, secs, pobj) in zip(todo, pool.map_async(task_verify, jobs, chunksize=1).get(TIMEOUT) if todo else []):
        p = Proved(s, obj, info, whole, lines, key, pobj=pobj)
        out[s] = p
        c = dict(key=key, obj=obj, info=info, whole=whole, lines=lines, pobj=pobj,
                 objsha=sha(open(obj, 'rb').read()) if obj and os.path.exists(obj) else None)
        with open(os.path.join(cdir, stem(s) + '.pkl'), 'wb') as f: pickle.dump(c, f)
    log(f'verify: {len(todo)} sources checked with armmatch in {time.time() - t:.0f}s' +
        (f', {len(srcs) - len(todo)} unchanged since the last run' if fast else ''))
    return out


# ---- layout -----------------------------------------------------------------------------------
@dataclass
class Layout:
    base: int
    end: int                       # end of the image
    table: tuple                   # region table (start, end)
    rw_end: int                    # end of the compressed RW data in flash (padded image end)
    sections: list                 # (stem, obj, section, addr, size) of every proved section
    flash: list                    # the same, in flash only
    placed: set = field(default_factory=set)        # stems proved by object placement (not in the pieces)
    pads: dict = field(default_factory=dict)        # end -> (start, stem before, stem after): alignment padding between two proved sections
    table_pad: tuple = None                         # (start, stem before): armlink's padding before the region table


def place_all(cfg, m, proved, log):
    problems = []
    secs = []
    for s, p in sorted(proved.items()):
        if p.info is None: continue          # a source that fails verify is still linked: its bytes must show
        text = open(s, encoding='latin1').read()
        placements, defined, errors, unplaced = armmatch.place(p.info, m, armmap.annotations(text),
                                                               armmap.unused_dropped(text))
        if errors: problems.append(f'{rel(s)}: ' + '; '.join(errors)); continue
        size_of = {n: z for n, z in p.info.sections.values()}
        p.placements = [(sec, a, size_of.get(sec) or 0) for sec, a in placements]
        p.defined = defined
        p.unplaced = unplaced
        for sec, a, z in p.placements:
            if z: secs.append((stem(s), p.obj, sec, a, z))
    secs.sort(key=lambda x: x[3])
    for x, y in zip(secs, secs[1:]):
        if x[3] + x[4] > y[3] and (x[3] >= 0x20000000) == (y[3] >= 0x20000000):
            problems.append(f'{x[0]}({x[2]}) 0x{x[3]:08X}+0x{x[4]:X} overlaps {y[0]}({y[2]}) 0x{y[3]:08X}')
    return secs, problems


def zero_pads(lay, img, aligns):
    """{end: (start, stem before, stem after)} of the gaps a linker leaves for alignment: 1 to 3
    zero bytes between two proved flash sections, ending where the second starts, on its
    alignment. Between two sections in a piece armlink makes them (the two share a region); next
    to a section proved by object placement the placement makes them, as armlink would."""
    out = {}
    secs = sorted(lay.flash, key=lambda x: x[3])
    for x, y in zip(secs, secs[1:]):
        e = x[3] + x[4]
        n = y[3] - e
        al = aligns.get((y[0], y[2]), 1)
        if 0 < n < 4 and y[3] % max(al, 1) == 0 and n < al and not any(img.bytes(e, n)):
            out[y[3]] = (e, x[0], y[0])
    return out


def table_pad(lay, img):
    """(start, stem) of the gap between the last proved flash section and the region table when it
    is 1 to 3 zero bytes: armlink's padding before Region$$Table, which holds words and so is
    4-byte aligned. The library link makes it (library_job) and its listing must show the PAD."""
    before = [x for x in lay.flash if x[3] + x[4] <= lay.table[0]]
    if not before: return None
    x = max(before, key=lambda x: x[3] + x[4])
    e = x[3] + x[4]
    n = lay.table[0] - e
    if 0 < n < 4 and lay.table[0] % 4 == 0 and not any(img.bytes(e, n)):
        return (e, x[0])
    return None


def placed_span(lay, a, z):
    """The bytes a placed section covers with the alignment padding before and after it."""
    s = next((p[0] for e, p in lay.pads.items() if e == a), a)
    e = a + z
    e = next((x for x, p in lay.pads.items() if p[0] == e), e)
    return s, e


def piece_bounds(lay, gc):
    """Cut points: a nominal grid every PIECE bytes from the image base, each moved down to the
    nearest 4-byte boundary that is inside no proved flash section (nor right after armlink's
    padding before one, which belongs with the section before it). A grid keeps the cuts where
    they were when a file elsewhere is proved, so --fast relinks only the pieces that changed.
    Sections proved by object placement are not in any piece: the pieces run around them."""
    inside = [(a, a + z) for _, _, _, a, z in lay.flash]

    def ok(c):
        return c % 4 == 0 and not any(a < c < e for a, e in inside) and c not in lay.pads
    holes = sorted(placed_span(lay, a, z) for st, _, _, a, z in lay.flash if st in lay.placed)
    out = []
    for s0, e0 in gaps(lay.base, lay.table[0], holes):
        cuts = [s0]
        c = lay.base + PIECE
        while c < e0:
            if c > s0:
                x = c & ~3
                while x > s0 and not ok(x): x -= 4
                if x > cuts[-1]: cuts.append(x)
            c += PIECE
        cuts.append(e0)
        # a large proved file over a grid point moves that cut down by its length and can push the
        # next piece over MDK-Lite's limit: such a piece gets one more cut, near its middle
        k = 0
        while k + 1 < len(cuts):
            s, e = cuts[k], cuts[k + 1]
            if e - s > PIECE + 0x800:
                m = (s + (e - s) // 2) & ~3
                x = m
                while x > s and not ok(x): x -= 4
                if x <= s:
                    x = m
                    while x < e and not ok(x): x += 4
                if s < x < e:
                    cuts.insert(k + 1, x)
                    continue
            k += 1
        out += list(zip(cuts, cuts[1:]))
    return out


def gaps(start, end, taken):
    """Runs of [start, end) that no (a, e) in taken covers."""
    out, at = [], start
    for a, e in sorted(taken):
        a, e = max(a, start), min(e, end)
        if e <= a: continue
        if a > at: out.append((at, a))
        at = max(at, e)
    if at < end: out.append((at, end))
    return out


# ---- scatter helpers ---------------------------------------------------------------------------
def flash_groups(segs, pads=None):
    """Contiguous segments (obj, sec, addr, size) grouped where a boundary is off a 4-byte
    boundary (armlink rounds a region's end up to 4, so they must share a region). A segment
    after alignment padding (pads {end: start}) shares the region of the one before it, so
    armlink makes the padding."""
    groups = []
    pads = pads or {}
    for s in sorted(segs, key=lambda x: x[2]):
        if groups:
            p = groups[-1][-1]
            if s[2] == p[2] + p[3] and (s[2] % 4 or (p[2] + p[3]) % 4):
                groups[-1].append(s); continue
            if pads.get(s[2]) == p[2] + p[3]:
                groups[-1].append(s); continue
        groups.append([s])
    return groups


def group_lines(groups, tag, pads=None):
    """Scatter lines: one FIXED region per group, its first section +FIRST and its last +LAST
    (armlink orders a region by attribute and name, not by the scatter file). A group that starts
    off a 4-byte boundary (a piece that starts after a section proved by object placement) goes
    after a pad, in a region at the boundary below, loaded out of the way; with pads None that is
    an error, else (name, bytes, kind) is appended to pads."""
    lines, errors = [], []
    for n, g in enumerate(groups, 1):
        a0 = g[0][2]
        # between +FIRST and +LAST, same-named sections keep the input list's order (the proved
        # objects, sorted by name): one source's .constdata after another's
        cap = 3 - (1 if a0 % 4 else 0)
        mid = g[1:-1]
        if len(g) > cap and (len({s for _, s, _, _ in mid}) > 1 or any(o.startswith('gate_') for o, *_ in mid)
                             or [o for o, *_ in mid] != sorted(o for o, *_ in mid)):
            errors.append(f'group at 0x{a0:08X} has {len(g)} sections (more than {cap} only when the middle ones '
                          f'are same-named sections of proved objects in name order)')
        sel = group_sel(g)
        if a0 % 4:
            if pads is None: errors.append(f'group at 0x{a0:08X} starts off a 4-byte boundary'); continue
            pads.append((f'{tag}{n}', a0 % 4, 'ro'))
            lines += [f'LR_{tag}{n} 0x{WORK_ASIDE + 0x10000 * (0x100 + n):08X} {{',
                      f'  ER_{tag}{n} 0x{a0 & ~3:08X} {{ gate_pad.o(.pad{tag}{n}, +FIRST) {" ".join(sel)} }}', '}']
            continue
        lines += [f'LR_{tag}{n} 0x{a0:08X} {{', f'  ER_{tag}{n} 0x{a0:08X} FIXED {{ {" ".join(sel)} }}', '}']
    return lines, errors


def group_sel(g, last=True):
    """A group's input section selectors, its first pinned +FIRST and its last +LAST (last=False
    leaves the last unpinned, for a region that holds something after it)."""
    a0 = g[0][2]
    sel = []
    for j, (o, s, a, z) in enumerate(g):
        pin = ', +FIRST' if j == 0 and len(g) > 1 and not a0 % 4 else \
              ', +LAST' if last and j == len(g) - 1 and len(g) > 1 + (0 if a0 % 4 else 1) else ''
        sel.append(f'{o}({s}{pin})')
    return sel


def blob_asm(blobs, kind='DATA, READONLY'):
    """An assembler source with one area per blob [(name, file, size)], its bytes by INCBIN."""
    out = []
    for name, f, z in blobs:
        out += [f'\tAREA |{name}|, {kind}, ALIGN=0', f'\tINCBIN {f}']
    return '\n'.join(out + ['\tEND']) + '\n'


# ---- stage 3: pieces ---------------------------------------------------------------------------
def task_link(job):
    """One link in a worker: job = dict(kind, work, files {name: text or bytes}, assemble [names],
    compile [names], inputs [paths], scatter, home). Returns dict(ok, log, bins, listing)."""
    tc = _W['tc'] if not job.get('home') else armtc.Toolchain(job['home'], ROOT)
    work = job['work']; os.makedirs(work, exist_ok=True)
    for n, data in job['files'].items():
        with open(os.path.join(work, n), 'wb' if isinstance(data, bytes) else 'w') as f: f.write(data)
    for n in job.get('assemble', []):
        ok, log = tc.assemble(os.path.join(work, n), os.path.join(work, stem(n) + '.o'), '--cpu Cortex-M3')
        if not ok: return dict(ok=False, log=f'{n}: assemble failed\n{log}')
    for n in job.get('compile', []):
        ok, log = tc.compile(os.path.join(work, n), os.path.join(work, stem(n) + '.o'), '--cpu Cortex-M3 -O0')
        if not ok: return dict(ok=False, log=f'{n}: compile failed\n{log}')
    axf = os.path.join(work, 'link.axf'); lst = os.path.join(work, 'link.lst')
    for p in (axf, lst):
        if os.path.exists(p): os.remove(p)
    args = ['--cpu', 'Cortex-M3', '--scatter', tc.inside(os.path.join(work, 'link.sct'))] + job['args'] + \
           ['--map', '--symbols', '--list', tc.inside(lst)] + [tc.inside(i) for i in job['inputs']] + ['-o', tc.inside(axf)]
    rc, log = tc.run('armlink', args, work)
    if rc or not os.path.exists(axf): return dict(ok=False, log=log)
    bins = tc.tobin(axf, os.path.join(work, 'bin'))
    return dict(ok=True, log=log, bins=bins, listing=open(lst, encoding='latin1').read())


_LMAP = re.compile(r'^\s*0x([0-9a-fA-F]{8})\s+0x([0-9a-fA-F]{8})\s+(Code|Data|Zero|PAD)\s+(RO|RW)\s+\d+\s+(?:\*\s+)?(\S+)\s+(\S+)\s*$')
_LSYM = re.compile(r'^\s{4}(\S+)\s+0x([0-9a-fA-F]{8})\s+(Thumb Code|ARM Code|Data|Number)\s+(\d+)\s+(\S+)$')
_EXEC = re.compile(r'Execution Region (\w+) \(Base: 0x([0-9a-fA-F]+)')


def listing_sections(text):
    """[(addr, size, section, object)] from an armlink --map listing (PAD rows left out)."""
    out = []
    for l in text.splitlines():
        m = _LMAP.match(l)
        if m and m.group(3) != 'PAD': out.append((int(m.group(1), 16), int(m.group(2), 16), m.group(5), m.group(6)))
    return out


def region_bases(text):
    return {m.group(1): int(m.group(2), 16) for m in _EXEC.finditer(text)}


def piece_job(cfg, gc, m, lay, proved, start, end, libsyms, extra_names):
    """The link of one piece: (job, segments [(kind, label, addr, size)], errors)."""
    work = os.path.join(GATE, 'pieces', f'{start:08X}')
    taken = [(a, a + z) for _, _, _, a, z in lay.flash if start <= a < end]
    in_piece = [x for x in lay.flash if start <= x[3] < end and x[0] not in lay.placed]
    apads = {e: p[0] for e, p in lay.pads.items() if start <= p[0] and e < end and
             p[1] not in lay.placed and p[2] not in lay.placed}
    taken += [(a, e) for e, a in apads.items()]
    if lay.table_pad and start <= lay.table_pad[0] < end:
        taken.append((lay.table_pad[0], lay.table[0]))          # made by the library link
    stems = sorted({x[0] for x in in_piece})
    by_stem = {stem(p.src): p for p in proved.values()}
    # every section of the piece's objects: those outside the piece go FIXED at their addresses
    other = [x for x in lay.sections if x[0] in stems and not (start <= x[3] < end)]
    holes = gaps(start, end, taken + gc['lib_ranges'])
    img = ehex.Image(cfg.image)
    files, blobs = {}, []
    for a, e in holes:
        fn = f'blob_{a:08X}.bin'
        files[fn] = img.bytes(a, e - a)
        blobs.append((f'.blob_{a:08X}', fn, e - a))
    if blobs: files['gate_blob.s'] = blob_asm(blobs)
    segs = [(os.path.basename(o), sec, a, z) for st, o, sec, a, z in in_piece] + \
           [('gate_blob.o', n, int(n[6:], 16), z) for n, _, z in blobs]
    if not segs: return None, [], []          # all library: nothing to link
    pads = []
    lines, errors = group_lines(flash_groups(segs, apads), 'P', pads)
    # outside the piece: each section its own region (a pad region when off a 4-byte boundary)
    for k, (st, o, sec, a, z) in enumerate(sorted(other, key=lambda x: x[3]), 1):
        ob = os.path.basename(o)
        if a % 4:
            pads.append((k, a % 4, armmap._kind(sec)))
            lines += [f'LR_O{k} 0x{WORK_ASIDE + 0x10000 * k:08X} {{',
                      f'  ER_O{k} 0x{a & ~3:08X} {{ gate_pad.o(.pad{k}, +FIRST) {ob}({sec}) }}', '}']
        else:
            lines += [f'LR_O{k} 0x{a:08X} {{', f'  ER_O{k} 0x{a:08X} FIXED {{ {ob}({sec}) }}', '}']
    if pads: files['gate_pad.s'] = pad_asm(pads)
    defined = set()
    for st in stems: defined |= set(by_stem[st].defined)
    extra = {n: a for n, a in extra_names.items() if n not in defined}
    entries = [e for e in m.symdef_entries(defined, extra)]
    have = {e[2] for e in entries} | defined
    entries += [(a, 'T', n) for n, a in libsyms.items() if n not in have]
    files['piece.sym'] = armmap.symdefs_text(entries) + extra_symdefs(defined | have)
    files['link.sct'] = '\n'.join(lines) + '\n'
    inputs = [by_stem[st].obj for st in stems] + [os.path.join(work, 'piece.sym')]
    if blobs: inputs.append(os.path.join(work, 'gate_blob.o'))
    if pads: inputs.append(os.path.join(work, 'gate_pad.o'))
    size = sum(z for *_, z in segs) + sum(x[4] for x in other if not sec_is_zi(by_stem[x[0]].info, x[2]))
    if size > LITE: errors.append(f'0x{size:X} bytes of RO+RW is over the MDK-Lite limit')
    job = dict(work=work, files=files, assemble=[n for n in ('gate_blob.s', 'gate_pad.s') if n in files],
               inputs=inputs, args=['--no_startup', '--keep', '*', '--datacompressor', 'off', '--no_scanlib',
                                    '--diag_suppress', '6305,6314,6329'])
    kinds = [('C', st, a, z, sec) for st, o, sec, a, z in in_piece] + [('blob', n, int(n[6:], 16), z, n) for n, _, z in blobs] + \
            [('pad', f'padding 0x{a:08X}', a, e - a, None) for e, a in apads.items()]
    return job, kinds, errors


def sec_is_zi(info, sec):
    """A zero-init section (armcc's .bss, or an assembler NOINIT area such as the startup file's
    STACK and HEAP): MDK-Lite does not count it."""
    idx = next((i for i, (n, _) in info.sections.items() if n == sec), None)
    return sec == '.bss' or (idx is not None and armmatch.is_zi(info, idx))


def pad_asm(pads):
    out = []
    for k, n, kind in pads:
        attr = 'DATA, READONLY' if kind == 'ro' else 'DATA, READWRITE' if kind == 'rw' else 'DATA, NOINIT'
        out += [f'\tAREA |.pad{k}|, {attr}, ALIGN=2', f'\tSPACE {n}']
    return '\n'.join(out + ['\tEND']) + '\n'


def extra_symdefs(defined):
    """A modded build's own symbols (GATE_EXTRA_SYMDEFS names a symdefs file), less the ones the link defines."""
    path = os.environ.get('GATE_EXTRA_SYMDEFS')
    if not path: return ''
    return ''.join(l for l in open(path) if l.split() and not l.startswith('#') and l.split()[-1] not in defined)


def job_key(job, objs):
    parts = [VERSION, job.get('home', ''), ' '.join(job['args'])]
    for n in sorted(job['files']): parts += [n, job['files'][n]]
    for o in objs: parts += [os.path.basename(o), open(o, 'rb').read() if os.path.exists(o) else b'missing']
    return sha(*parts)


def run_links(jobs, pool, fast, log):
    """{name: result} for {name: job}, reusing cached results under --fast."""
    cdir = os.path.join(GATE, 'cache', 'link'); os.makedirs(cdir, exist_ok=True)
    out, todo = {}, []
    for name, job in jobs.items():
        objs = [i for i in job['inputs'] if not i.startswith(job['work'])]
        key = job_key(job, objs)
        cp = os.path.join(cdir, name + '.pkl')
        if fast and os.path.exists(cp):
            with open(cp, 'rb') as f: c = pickle.load(f)
            if c['key'] == key: out[name] = dict(c['res'], cached=True); continue
        todo.append((name, key, job))
    t = time.time()
    for (name, key, job), res in zip(todo, pool.map_async(task_link, [j for _, _, j in todo], chunksize=1).get(TIMEOUT) if todo else []):
        out[name] = res
        if res.get('ok'):
            with open(os.path.join(cdir, name + '.pkl'), 'wb') as f: pickle.dump(dict(key=key, res=res), f)
    log(f'links: {len(todo)} linked in {time.time() - t:.0f}s' + (f', {len(jobs) - len(todo)} unchanged since the last run' if fast else ''))
    return out


# ---- stage 2: the library link -----------------------------------------------------------------
def library_job(cfg, gc, lay):
    """The startup code, the library members, the region table and the RW image, linked from the
    library as the original link did."""
    work = os.path.join(GATE, 'library')
    img = ehex.Image(cfg.image)
    regions = rwdata.find_table(img)
    ram = rwdata.RamImage(img, regions)
    rw = [r for r in regions if r.kind != 'zero']
    zi = [r for r in regions if r.kind == 'zero']
    if len(rw) != 1 or len(zi) != 1 or rw[0].exec + rw[0].size != zi[0].exec:
        raise SystemExit('the gate expects one RW region followed by one zero-init region')
    rw, zi = rw[0], zi[0]
    zi_end = zi.exec + zi.size
    files = {'rw.bin': ram.bytes(rw.exec, rw.size)}
    # zero-init: the application's part, the library's __libspace (its .bss, at its own address),
    # then the heap and stack (areas named after .bss, so they sort after the library's)
    s = [f'\tAREA |.data|, DATA, READWRITE, ALIGN=2', '\tINCBIN rw.bin',
         f'\tAREA |.bss|, DATA, NOINIT, ALIGN=2', f'\tSPACE 0x{gc["libspace"] - zi.exec:X}',
         f'\tAREA |HEAP|, DATA, NOINIT, ALIGN=3', f'\tSPACE 0x{zi_end - gc["heap"]:X}', '\tEND']
    files['ram.s'] = '\n'.join(s) + '\n'
    # the stub asks for what the application's objects called, in the order armlink resolves them
    req = gc['lib_requests']
    stub = ['\tAREA |.text|, CODE, READONLY, ALIGN=2', '\tTHUMB'] + [f'\tIMPORT {n}' for n in req] + \
           ['gate_lib_stub'] + [f'\tDCD {n}' for n in req] + ['\tEND']
    files['gate_stub.s'] = '\n'.join(stub) + '\n'
    files['lib.sym'] = armmap.symdefs_text([(a, 'D' if a < 0x08000000 else 'T', n) for n, a in gc['lib_symdefs'].items()])
    (m0, m1), (t0, t1) = gc['lib_ranges'][0], gc['lib_ranges'][1]
    aside = (lay.end + 0xFFFF) & ~0xFFFF
    # the padding before the region table (table_pad): the table's region starts at the 4-byte
    # boundary below, with a stand-in for the end of the last proved section (a byte-aligned area
    # of that many bytes, left out of the image); armlink then aligns Region$$Table and pads
    tb, keep, inputs = lay.table[0], [], []
    if lay.table_pad:
        tb = lay.table[0] - 4
        files['gate_lead.s'] = f'\tAREA |.lead|, DATA, READONLY, ALIGN=0\n\tSPACE {lay.table_pad[0] - tb}\n\tEND\n'
        keep, inputs = ['--keep', 'gate_lead.o(*)'], [os.path.join(work, 'gate_lead.o')]
    files['link.sct'] = f"""LR_MAIN 0x{m0:08X} {{
  ER_MAIN 0x{m0:08X} FIXED {{ *.l (!*, .ARM.Collect$$*) }}
}}
LR_TAIL 0x{t0:08X} {{
  ER_TAIL 0x{t0:08X} FIXED {{ *.l (.text, x$*) }}
}}
LR_DATA 0x{tb:08X} {{
  ER_TABLE 0x{tb:08X} FIXED {{ {'gate_lead.o(.lead, +FIRST) ' if lay.table_pad else ''}*(Region$$Table) }}
  ER_RAM 0x{rw.exec:08X} {{ ram.o (+RW +ZI) *.l (.data, .bss) }}
}}
LR_STUB 0x{aside:08X} {{
  ER_STUB 0x{aside:08X} {{ gate_stub.o (+RO) }}
}}
"""
    job = dict(work=work, files=files, assemble=['ram.s', 'gate_stub.s'] + (['gate_lead.s'] if lay.table_pad else []),
               home=gc['libhome'], inputs=[os.path.join(work, n) for n in ('gate_stub.o', 'ram.o', 'lib.sym')] + inputs,
               args=['--keep', 'gate_stub.o(*)', '--keep', 'ram.o(*)'] + keep + ['--entry', '__main',
                     '--libpath', os.path.join(gc['libhome'], 'ARM', 'ARMCC', 'lib'),
                     '--diag_suppress', '6305,6314,6329'])
    return job


# ---- the gate ----------------------------------------------------------------------------------
def main(argv):
    if '--whole' in argv: return main_whole(argv)
    fast = '--fast' in argv
    t0 = time.time()
    if os.environ.get('GATE_DEBUG'):
        import faulthandler; faulthandler.dump_traceback_later(int(os.environ['GATE_DEBUG']), exit=True)
    cfg = load_cfg()
    gc = gate_cfg(cfg)
    if '--jobs' in argv: gc['jobs'] = int(argv[argv.index('--jobs') + 1])
    if '--libhome' in argv: gc['libhome'] = os.path.join(ROOT, argv[argv.index('--libhome') + 1])
    place = os.environ.get('GATE_PLACE', '')
    if '--place' in argv: place = argv[argv.index('--place') + 1]
    place = {x.strip() for x in place.split(',') if x.strip()}
    with_srcs = [argv[i + 1] for i, x in enumerate(argv) if x == '--with']
    gc['jobs'] = max(1, min(gc['jobs'], 4))
    os.makedirs(GATE, exist_ok=True)
    report = []

    def log(s=''):
        print(s, flush=True); report.append(s)
    m = armmap.Map.from_dir(cfg.map)
    mapkey = sha(open(os.path.join(cfg.map, 'functions.tsv'), 'rb').read(), open(os.path.join(cfg.map, 'data.tsv'), 'rb').read())
    srcs = sources(cfg, with_srcs)
    img = ehex.Image(cfg.image)
    t_lo, t_hi = rwdata._table_rt2(img)
    lay = Layout(img.base, img.end, (t_lo, t_hi), img.end, [], [])
    ok = True
    table_pad_bytes = None
    pool = LazyPool(gc['jobs'])
    try:
        proved = verify_all(cfg, srcs, pool, fast, mapkey, log, place)
        bad = [p for p in proved.values() if not p.whole]
        for p in sorted(proved.values(), key=lambda p: p.src):
            if not p.whole:
                log(f'FAIL {rel(p.src)}: not a whole match')
                for l in p.lines[-12:]: log('    ' + l)
        log(f'verify: {len(proved) - len(bad)} of {len(proved)} sources match whole')
        if bad: ok = False
        if '--verify-only' in argv:
            log('-- gate (verify only): ' + ('PASS' if ok else 'FAIL') + f' in {time.time() - t0:.0f}s')
            return 0 if ok else 1
        secs, problems = place_all(cfg, m, proved, log)
        for x in problems: log('FAIL ' + x); ok = False
        lay.sections = secs
        lay.flash = [x for x in secs if lay.base <= x[3] < lay.end]
        lay.placed = {stem(p.src) for p in proved.values() if p.pobj is not None and p.placements}
        aligns = {}
        for p in proved.values():
            if p.info is not None:
                for i, (n, _) in p.info.sections.items(): aligns[(stem(p.src), n)] = (p.info.align or {}).get(i, 1)
        lay.pads = zero_pads(lay, img, aligns)
        lay.table_pad = table_pad(lay, img)
        libjob = library_job(cfg, gc, lay)
        for p in sorted(proved.values(), key=lambda p: p.src):
            if stem(p.src) in lay.placed:
                log(f'{rel(p.src)}: proved by object placement, left out of the pieces')
        # names that sources place with at: lines but the map does not know
        extra_names = {}
        for p in proved.values():
            if p.info is not None:
                for n, a in armmap.annotations(open(p.src, encoding='latin1').read()).items():
                    if n not in m.by_name: extra_names[n] = a
        # stage 2 first: the pieces take the library's symbols from it
        res = run_links({'library': libjob}, pool, fast, log)
        lib = res['library']
        libsyms, lib_bytes = {}, {}
        if not lib.get('ok'):
            log('FAIL library link:\n    ' + lib.get('log', '').strip().replace('\n', '\n    ')); ok = False
        else:
            for l in lib['listing'].splitlines():
                mm = _LSYM.match(l)
                if mm and mm.group(3) in ('Thumb Code', 'ARM Code'):
                    a = int(mm.group(2), 16)
                    if any(x <= (a & ~1) < y for x, y in gc['lib_ranges']): libsyms[mm.group(1)] = a & ~1
            bases = region_bases(lib['listing'])
            for name in ('ER_MAIN', 'ER_TAIL'):
                lib_bytes[bases[name]] = lib['bins'][name]
            # the region table at its address; before it, armlink's padding (table_pad), which the
            # listing must show as a PAD row from the end of the last proved section to the table
            tb = lib['bins']['ER_TABLE'][lay.table[0] - bases['ER_TABLE']:]
            lib_bytes[lay.table[0]] = tb
            got = {(a, sec) for a, z, sec, obj in listing_sections(lib['listing'])}
            if (lay.table[0], 'Region$$Table') not in got:
                log(f'FAIL library link: Region$$Table is not at 0x{lay.table[0]:08X}'); ok = False
            if lay.table_pad:
                a, n = lay.table_pad[0], lay.table[0] - lay.table_pad[0]
                if not re.search(rf'^\s*0x{a:08x}\s+0x{n:08x}\s+PAD\s*$', lib['listing'], re.M):
                    log(f'FAIL library link: no {n}-byte PAD at 0x{a:08X} before Region$$Table'); ok = False
                else:
                    table_pad_bytes = lib['bins']['ER_TABLE'][a - bases['ER_TABLE']:lay.table[0] - bases['ER_TABLE']]
        pieces = piece_bounds(lay, gc)
        jobs, kinds = {}, {}
        for s, e in pieces:
            job, k, errs = piece_job(cfg, gc, m, lay, proved, s, e, libsyms, extra_names)
            for x in errs: log(f'FAIL piece 0x{s:08X}-0x{e:08X}: {x}'); ok = False
            if job is None: continue
            jobs[f'piece_{s:08X}'] = job; kinds[f'piece_{s:08X}'] = (s, e, k)
        res.update(run_links(jobs, pool, fast, log))
    finally:
        pool.close()

    # ---- compose the image and compare ----
    want = img.data
    full = bytearray(len(want)); have = bytearray(len(want))      # have: 1 C, 2 library, 3 linker data, 4 blob
    owner = {}
    for name, (s, e, k) in kinds.items():
        r = res.get(name, {})
        if not r.get('ok'):
            log(f'FAIL {name} 0x{s:08X}-0x{e:08X}: link failed\n    ' + r.get('log', '').strip()[-3000:].replace('\n', '\n    '))
            ok = False; continue
        bases = region_bases(r['listing'])
        placed = {(sec, obj): a for a, z, sec, obj in listing_sections(r['listing'])}
        out = bytearray(e - s); got_mask = bytearray(e - s)
        for reg, b in r['bins'].items():
            rb = bases.get(reg)
            if rb is None or not reg.startswith('ER_P'): continue
            for i, x in enumerate(b):
                if s <= rb + i < e and rb + i < s + len(out):
                    out[rb + i - s] = x; got_mask[rb + i - s] = 1
        nbad = []
        for kind, label, a, z, sec in k:
            if kind == 'C':
                obj = label + '.o'
                at = placed.get((sec, obj))
                if at != a: nbad.append(f'{label}({sec}) linked at {at and hex(at)}, not 0x{a:08X}'); continue
            seg = bytes(out[a - s:a - s + z]); orig = want[a - lay.base:a - lay.base + z]
            if seg != orig or not all(got_mask[a - s:a - s + z]):
                diff = [i for i in range(z) if seg[i] != orig[i]]
                nbad.append(f'{label}({sec}) 0x{a:08X}: {len(diff)} bytes differ' + (f', first at 0x{a + diff[0]:08X}' if diff else ''))
            for i in range(a - lay.base, a - lay.base + z):
                if have[i] in (0, 4):
                    full[i] = out[i - (s - lay.base)]; have[i] = {'C': 1, 'blob': 4, 'pad': 6}[kind]
                    if kind == 'C': owner[i] = label
        state = 'MATCHES' if not nbad else 'DIFFERS'
        nC = sum(1 for x in k if x[0] == 'C'); nP = sum(1 for x in k if x[0] == 'pad')
        log(f'piece 0x{s:08X}-0x{e:08X} ({nC} proved sections, {len(k) - nC - nP} blobs' + (f', {nP} paddings' if nP else '') + ')' +
            (' [unchanged]' if r.get('cached') else '') + f': {state}')
        for x in nbad: log('    ' + x)
        if nbad: ok = False
    # sources proved by object placement: their bytes from the placement, with the pieces' symbols
    for p in sorted(proved.values(), key=lambda p: p.src):
        st = stem(p.src)
        if st not in lay.placed: continue
        extra = {n: a for n, a in extra_names.items() if n not in p.defined}
        entries = m.symdef_entries(set(p.defined), extra)
        have_n = {x[2] for x in entries} | set(p.defined)
        entries += [(a, 'T', n) for n, a in libsyms.items() if n not in have_n]
        errs = []
        try:
            table = objplace.symbol_table(entries)
        except ValueError as e:
            table = {}; errs.append(str(e))
        bases, e2 = objplace.bases_for(p.pobj, p.info, [(sec, a) for sec, a, _ in p.placements], p.unplaced)
        errs += e2
        out, e3, count = objplace.link(p.pobj, bases, table) if not errs else ({}, [], 0)
        errs += e3
        idx_of = {sec.name: i for i, sec in p.pobj.sections.items()}
        nbad, nbytes = [], 0
        for sec, a, z in p.placements:
            if not (lay.base <= a < lay.end) or errs: continue
            b = out[idx_of[sec]]
            orig = want[a - lay.base:a - lay.base + len(b)]
            diff = [i for i in range(len(b)) if i >= len(orig) or b[i] != orig[i]]
            if diff: nbad.append(f'{st}({sec}) 0x{a:08X}: {len(diff)} bytes differ, first at 0x{a + diff[0]:08X}')
            for i, x in enumerate(b):
                j = a - lay.base + i
                if j < len(full) and have[j] in (0, 4): full[j] = x; have[j] = 5; owner[j] = st
            nbytes += len(b)
            # alignment padding next to it (zero_pads): made here, as armlink would
            s0, e0 = placed_span(lay, a, len(b))
            for j in list(range(s0, a)) + list(range(a + len(b), e0)):
                if have[j - lay.base] == 0: full[j - lay.base] = 0; have[j - lay.base] = 6
        for x in errs: nbad.append(x)
        log(f'placed {rel(p.src)}: {len(p.placements)} sections, {nbytes} bytes in flash, {count} relocations resolved: ' +
            ('MATCHES' if not nbad else 'DIFFERS'))
        for x in nbad[:20]: log('    ' + x)
        if nbad: ok = False
    if lib_bytes:
        for rb, b in lib_bytes.items():
            if rb == lay.table[0]:
                kind = 3; n = len(want) - (rb - lay.base)          # the table and compressed RW data to the end
                b = b[:n]
                if len(b) != n: log(f'FAIL library link: the RW data is 0x{len(b):X} bytes, the image has 0x{n:X}'); ok = False
            else:
                kind = 2
            orig = want[rb - lay.base:rb - lay.base + len(b)]
            diff = [i for i in range(min(len(b), len(orig))) if b[i] != orig[i]]
            if diff or len(b) != len(orig):
                log(f'FAIL library link 0x{rb:08X}+0x{len(b):X}: {len(diff)} bytes differ' + (f', first at 0x{rb + diff[0]:08X}' if diff else '')); ok = False
            for i, x in enumerate(b):
                j = rb - lay.base + i
                if j >= len(full): break
                if have[j] in (1, 5):
                    if full[j] != x: log(f'FAIL 0x{lay.base + j:08X}: the library and {owner[j]} disagree'); ok = False
                    continue
                full[j] = x; have[j] = kind
        if lay.table_pad and table_pad_bytes is not None:
            a = lay.table_pad[0]
            orig = want[a - lay.base:lay.table[0] - lay.base]
            if table_pad_bytes != orig or any(table_pad_bytes):
                log(f'FAIL library link: the padding at 0x{a:08X} is {table_pad_bytes.hex()}, the image has {orig.hex()}'); ok = False
            for i, x in enumerate(table_pad_bytes):
                j = a - lay.base + i
                if have[j] == 0: full[j] = x; have[j] = 6
        log('library link' + (' [unchanged]' if lib.get('cached') else '') + ': ' +
            ', '.join(f'0x{rb:08X}-0x{rb + len(b):08X}' for rb, b in sorted(lib_bytes.items())) +
            (f' (with its padding 0x{lay.table_pad[0]:08X}-0x{lay.table[0]:08X} before the region table)'
             if lay.table_pad and table_pad_bytes is not None else ''))
    missing = gaps(0, len(full), [(i, i + 1) for i in range(len(have)) if have[i]]) if not all(have) else []
    if missing:
        ok = False
        log('FAIL not rebuilt: ' + ', '.join(f'0x{lay.base + a:08X}-0x{lay.base + b:08X}' for a, b in missing[:8]))
    out_img = os.path.join(GATE, 'A319.img'); open(out_img, 'wb').write(full)
    same = bytes(full) == want
    log(f'image: {rel(out_img)} (0x{lay.base:08X}-0x{lay.end:08X}): ' + ('IDENTICAL to the original image' if same else
        f'{sum(1 for x, y in zip(full, want) if x != y)} bytes differ'))
    if not same: ok = False
    header = struct.pack('<HH', gc['version'], gc['magic'])
    out_bin = os.path.join(GATE, 'A319_ARMStore.bin'); open(out_bin, 'wb').write(header + bytes(full))
    orig_bin = open(gc['bin'], 'rb').read()
    log(f'bin: {rel(out_bin)} (header version {gc["version"]}, magic 0x{gc["magic"]:04X}): ' +
        ('IDENTICAL to ' + rel(gc['bin']) if header + bytes(full) == orig_bin else 'DIFFERS from ' + rel(gc['bin'])))
    if header + bytes(full) != orig_bin: ok = False
    hex_text = ehex.write(lay.base, bytes(full), like=cfg.image).encode('ascii')
    out_hex = os.path.join(GATE, 'A319.hex'); open(out_hex, 'wb').write(hex_text)
    log(f'hex: {rel(out_hex)}: ' + ('IDENTICAL to ' + rel(cfg.image) if hex_text == open(cfg.image, 'rb').read() else 'differs from ' + rel(cfg.image)))
    coverage(log, lay, have, proved, srcs)
    log(f'-- gate: {"PASS" if ok else "FAIL"} in {time.time() - t0:.0f}s' + (' (--fast)' if fast else ''))
    with open(os.path.join(GATE, 'report.txt'), 'w') as f: f.write('\n'.join(report) + '\n')
    return 0 if ok else 1


# ---- --whole: the whole image in one link, with a licensed MDK ----------------------------------
def stand_ins(lay, gc):
    """Stems of proved sources whose flash sections all lie in the library's ranges: C stand-ins
    for library members (src/clib). A whole link takes the members from the library, so it leaves
    these out and compares their bytes with the members' afterwards."""
    by = {}
    for st, o, sec, a, z in lay.flash: by.setdefault(st, []).append((a, z))
    return {st for st, v in by.items() if all(any(x <= a and a + z <= y for x, y in gc['lib_ranges']) for a, z in v)}


def link_order(lay, names):
    """The proved stems in the original's link order: by the address of each one's .text (else its
    first flash section). armlink lays out sections of one name in input order, and the .text and
    the .constdata of the objects that have both run in the same order; the startup file's RESET
    is pinned first, so its .text, in the middle of the code, gives its place."""
    first, text = {}, {}
    for st, o, sec, a, z in lay.flash:
        first[st] = min(first.get(st, a), a)
        if sec == '.text': text[st] = a
    first.update(text)
    return sorted(names, key=lambda st: (first.get(st, 1 << 40), st))


_PLACEHOLDER = re.compile(r'(?:sub|g|c)_([0-9A-Fa-f]{8})$')


def resolve_refs(m, objs_info, extra_names, gc, base, end):
    """How the whole link resolves what the objects use and do not define:
    (symdefs entries [(addr, kind, name)] for RAM and peripheral names, steering lines
    ['RESOLVE alias AS name'] for a placeholder or map name of a flash address that a linked object
    defines under its own name, notes, errors). A name in a library range, or not in the map at all
    (__main, Lib$$Request$$armlib), is left to armlink and its libraries. No flash address is ever
    given as a number."""
    defined, refs = set(), set()
    for info in objs_info:
        for y in info.symbols:
            if y.sec == 'Ref': refs.add(y.name)
            elif y.bind in ('Gb', 'Wk'): defined.add(y.name)
    at = {}
    for r in m.functions + m.data: at.setdefault(r.addr, []).append(r.name)
    entries, steer, errors = [], [], []
    for n in sorted(refs - defined):
        row = m.by_name.get(n)
        a = row.addr if row else extra_names.get(n)
        if a is None:
            mm = _PLACEHOLDER.match(n)
            a = int(mm.group(1), 16) if mm else None
        if a is None: continue                                 # the library's or the linker's
        if 0x08000000 <= a < 0x08100000:
            if any(x <= a < y for x, y in gc['lib_ranges']): continue
            to = [x for x in at.get(a, []) if x in defined and x != n]
            if base <= a < end and to: steer.append(f'RESOLVE {n} AS {to[0]}')
            else: errors.append(f'{n} is flash address 0x{a:08X}, which no linked object defines by name')
        else:
            entries.append((a, 'T' if row is not None and n in m.code_names else 'D', n))
    return entries, steer, errors


def ram_regions(img):
    regions = rwdata.find_table(img)
    rw = [r for r in regions if r.kind != 'zero']
    zi = [r for r in regions if r.kind == 'zero']
    if len(rw) != 1 or len(zi) != 1 or rw[0].exec + rw[0].size != zi[0].exec:
        raise SystemExit('the gate expects one RW region followed by one zero-init region')
    return regions, rw[0], zi[0]


def ram_parts(img, gc, ram_secs):
    """The RW execution region as the original's: the image's own initial RAM in blobs around the
    proved sources' RAM sections (each at its address), the zero-init space up to the library's
    __libspace as a blob, then the library's .bss and the proved zero-init sections (the startup
    file's HEAP and STACK). ram_secs: [(addr, size, stem, obj, section)].
    Returns (files {name: text or bytes}, lead objects, tail objects, region selectors, checks
    [(what, addr, section, object)], errors). Lead objects go before every proved object in the
    input list and tail ones after, so that sections of one name keep address order."""
    regions, rw, zi = ram_regions(img)
    ram = rwdata.RamImage(img, regions)
    zi_end = zi.exec + zi.size
    files, lead, tail, sel, checks, errors = {}, [], [], [], [], []
    rws = [x for x in ram_secs if rw.exec <= x[0] < rw.exec + rw.size]
    zis = [x for x in ram_secs if zi.exec <= x[0] < zi_end]
    for x in ram_secs:
        if x not in rws and x not in zis: errors.append(f'{x[2]}({x[4]}) at 0x{x[0]:08X} is outside the RW and ZI regions')
    holes = gaps(rw.exec, rw.exec + rw.size, [(a, a + z) for a, z, *_ in rws])
    for k, (a, e) in enumerate(holes):
        name = f'gate_rw{k}'
        files[name + '.bin'] = ram.bytes(a, e - a)
        files[name + '.s'] = f'\tAREA |.data|, DATA, READWRITE, ALIGN={2 if a % 4 == 0 else 0}\n\tINCBIN {name}.bin\n\tEND\n'
        if rws and rws[0][0] < a < rws[-1][0]: errors.append(f'RW blob 0x{a:08X} lies between two proved RW sections')
        (lead if not rws or a < rws[0][0] else tail).append(name + '.o')
        sel.append(f'{name}.o(.data)'); checks.append(('RW data blob', a, '.data', name + '.o'))
    first_zi = min([gc['libspace']] + [a for a, *_ in zis])
    if any(a < gc['libspace'] for a, *_ in zis): errors.append('a proved zero-init section lies below the library\'s __libspace')
    files['gate_zi0.s'] = f'\tAREA |.bss|, DATA, NOINIT, ALIGN=2\n\tSPACE 0x{first_zi - zi.exec:X}\n\tEND\n'
    tail.append('gate_zi0.o'); sel.append('gate_zi0.o(.bss)'); checks.append(('ZI blob', zi.exec, '.bss', 'gate_zi0.o'))
    for a, z, st, o, sec in rws + zis:
        sel.append(f'{os.path.basename(o)}({sec})'); checks.append((f'{st}({sec})', a, sec, os.path.basename(o)))
    sel.append('*.l (.data, .bss)')
    return files, lead, tail, sel, checks, errors, rw, zi


def whole_job(cfg, gc, m, lay, proved, extra_names, home):
    """The scatter file, inputs and checks of one armlink run that makes the whole image."""
    work = os.path.join(GATE, 'link')
    img = ehex.Image(cfg.image)
    skip = stand_ins(lay, gc)
    by_stem = {stem(p.src): p for p in proved.values() if p.info is not None and p.placements}
    order = link_order(lay, [st for st in by_stem if st not in skip])
    errors = []
    segs = [(os.path.basename(o), sec, a, z) for st, o, sec, a, z in lay.flash if st in order]
    apads = {e: p[0] for e, p in lay.pads.items() if p[1] in order and p[2] in order}
    rows, table_in = [], None
    tpad = lay.table_pad[0] if lay.table_pad else lay.table[0]
    for n, g in enumerate(flash_groups(segs, apads), 1):
        a0, end = g[0][2], g[-1][2] + g[-1][3]
        if a0 % 4: errors.append(f'group at 0x{a0:08X} starts off a 4-byte boundary')
        sel = group_sel(g, last=end != tpad)
        if end == tpad:
            sel.append('*(Region$$Table)'); table_in = f'ER_G{n}'      # armlink aligns it and pads, as in the original
        rows.append((a0, f'  ER_G{n} 0x{a0:08X} FIXED {{ {" ".join(sel)} }}'))
    (m0, m1), (t0, t1) = gc['lib_ranges'][0], gc['lib_ranges'][1]
    rows.append((m0, f'  ER_LIBMAIN 0x{m0:08X} FIXED {{ *.l (!*, .ARM.Collect$$*) }}'))
    rows.append((t0, f'  ER_LIBTAIL 0x{t0:08X} FIXED {{ *.l (.text, x$*) }}'))
    if table_in is None:
        rows.append((lay.table[0], f'  ER_TABLE 0x{lay.table[0]:08X} FIXED {{ *(Region$$Table) }}'))
    rows.sort()
    ram_secs = sorted((a, z, st, o, sec) for st, o, sec, a, z in lay.sections if a >= 0x20000000 and st in order)
    files, lead, tail, rsel, rchecks, rerr, rw, zi = ram_parts(img, gc, ram_secs)
    errors += rerr
    entries, steer, serr = resolve_refs(m, [by_stem[st].info for st in order], extra_names, gc, lay.base, lay.end)
    errors += serr
    files['whole.sym'] = armmap.symdefs_text(entries)
    files['whole.steer'] = '\n'.join(steer) + '\n'
    files['link.sct'] = '\n'.join([f'LR_IROM1 0x{lay.base:08X} {{'] + [r for _, r in rows] +
                                  [f'  RW_IRAM1 0x{rw.exec:08X} {{ {" ".join(rsel)} }}', '}']) + '\n'
    objs = [by_stem[st].obj for st in order]
    inputs = [os.path.join(work, o) for o in lead] + objs + [os.path.join(work, o) for o in tail] + [os.path.join(work, 'whole.sym')]
    keep = []
    for o in objs + lead + tail: keep += ['--keep', os.path.basename(o) + '(*)']
    job = dict(work=work, files=files, assemble=[n for n in files if n.endswith('.s')], home=home, inputs=inputs,
               args=keep + ['--entry', 'Reset_Handler', '--edit', os.path.join(work, 'whole.steer'),
                            '--libpath', os.path.join(gc['libhome'], 'ARM', 'ARMCC', 'lib'),
                            '--diag_suppress', '6305,6314,6329,6332'])     # L6332W: an alias resolved by the steering file
    checks = [(f'{st}({sec})', a, sec, os.path.basename(o)) for st, o, sec, a, z in lay.flash if st in order] + rchecks
    checks.append(('Region$$Table', lay.table[0], 'Region$$Table', None))
    checks.append(('library __libspace', gc['libspace'], '.bss', 'c_w.l(libspace.o)'))
    return job, checks, errors, skip, order, steer, entries


def main_whole(argv):
    """gate.py --whole: verify every source with the licensed MDK (armlink, no object placement),
    then link the whole image in one armlink run and compare it."""
    global GATE
    t0 = time.time()
    home = a319env.require_licensed('gate.py --whole')
    GATE = os.environ['GATE_DIR'] = os.path.join(ROOT, 'build', 'whole')
    os.environ['GATE_WHOLE'] = '1'
    os.environ['MDK_HOME'] = a319env.MDK_HOME = home          # the workers compile and link with it too
    fast = '--fast' in argv
    cfg = load_cfg()
    gc = gate_cfg(cfg)
    if '--jobs' in argv: gc['jobs'] = int(argv[argv.index('--jobs') + 1])
    gc['jobs'] = max(1, min(gc['jobs'], 4))
    with_srcs = [argv[i + 1] for i, x in enumerate(argv) if x == '--with']
    os.makedirs(GATE, exist_ok=True)
    report = []

    def log(s=''):
        print(s, flush=True); report.append(s)
    log('gate --whole: verify and link with the licensed MDK (one armlink run for the whole image)')
    m = armmap.Map.from_dir(cfg.map)
    mapkey = sha(open(os.path.join(cfg.map, 'functions.tsv'), 'rb').read(), open(os.path.join(cfg.map, 'data.tsv'), 'rb').read())
    srcs = sources(cfg, with_srcs)
    img = ehex.Image(cfg.image)
    t_lo, t_hi = rwdata._table_rt2(img)
    lay = Layout(img.base, img.end, (t_lo, t_hi), img.end, [], [])
    ok = True
    a319env.start_licensed()
    pool = LazyPool(gc['jobs'])
    try:
        proved = verify_all(cfg, srcs, pool, fast, mapkey, log)
    finally:
        pool.close()
    try:
        bad = [p for p in proved.values() if not p.whole]
        for p in sorted(bad, key=lambda p: p.src):
            log(f'FAIL {rel(p.src)}: not a whole match')
            for l in p.lines[-12:]: log('    ' + l)
        log(f'verify: {len(proved) - len(bad)} of {len(proved)} sources match whole (armlink for every one, no object placement)')
        if bad: ok = False
        if any(p.pobj is not None for p in proved.values()):
            log('FAIL a source was proved by object placement'); ok = False
        secs, problems = place_all(cfg, m, proved, log)
        for x in problems: log('FAIL ' + x); ok = False
        lay.sections = secs
        lay.flash = [x for x in secs if lay.base <= x[3] < lay.end]
        aligns = {}
        for p in proved.values():
            if p.info is not None:
                for i, (n, _) in p.info.sections.items(): aligns[(stem(p.src), n)] = (p.info.align or {}).get(i, 1)
        lay.pads = zero_pads(lay, img, aligns)
        lay.table_pad = table_pad(lay, img)
        extra_names = {}
        for p in proved.values():
            if p.info is not None:
                for n, a in armmap.annotations(open(p.src, encoding='latin1').read()).items():
                    if n not in m.by_name: extra_names[n] = a
        job, checks, errors, skip, order, steer, entries = whole_job(cfg, gc, m, lay, proved, extra_names, home)
        for x in errors: log('FAIL whole link: ' + x); ok = False
        log(f'whole link: {len(order)} proved objects, {sum(1 for c in checks if c[3] and not c[3].startswith("gate_"))} sections '
            f'at their addresses, {len(steer)} aliases resolved to their names, {len(entries)} RAM and peripheral names from the map; '
            f'left to the library: {", ".join(sorted(skip)) or "nothing"} (C stand-ins for library members)')
        res = run_links({'whole': job}, _InProcess(), fast, log)['whole']
    finally:
        a319env.stop_licensed()
    if not res.get('ok'):
        log('FAIL whole link:\n    ' + res.get('log', '').strip()[-4000:].replace('\n', '\n    '))
        log(f'-- gate --whole: FAIL in {time.time() - t0:.0f}s')
        return 1
    listing = res['listing']
    got = {}
    for a, z, sec, obj in listing_sections(listing): got.setdefault((sec, obj), []).append(a)
    nbad = []
    for what, a, sec, obj in checks:
        hits = [x for (s_, o_), v in got.items() for x in v if s_ == sec and (obj is None or o_ == obj)]
        if a not in hits: nbad.append(f'{what} linked at {", ".join(hex(x) for x in hits) or "nowhere"}, not 0x{a:08X}')
    for x in nbad: log('FAIL ' + x)
    if nbad: ok = False
    log(f'listing: {len(checks) - len(nbad)} of {len(checks)} sections and blobs at their addresses (Region$$Table at '
        f'0x{lay.table[0]:08X}, the RW blobs, __libspace, HEAP and STACK included)')
    # the image: the load view of the one load region, from fromelf
    full = whole_image(res, lay)
    want = img.data
    if len(full) != len(want):
        log(f'FAIL the link is 0x{len(full):X} bytes, the image 0x{len(want):X}'); ok = False
    full = (bytes(full) + bytes(len(want)))[:len(want)]
    # every flash byte up to the end of the region table is a section or armlink's alignment PAD:
    # no gap that a FIXED region's load padding would fill
    rows = []
    for l in listing.splitlines():
        mm = _LMAP.match(l) or re.match(r'^\s*0x([0-9a-fA-F]{8})\s+0x([0-9a-fA-F]{8})\s+(PAD)\s*$', l)
        if mm and lay.base <= int(mm.group(1), 16) < lay.table[1]:
            rows.append((int(mm.group(1), 16), int(mm.group(1), 16) + int(mm.group(2), 16), mm.group(3)))
    holes = gaps(lay.base, lay.table[1], [(a, e) for a, e, k in rows])
    pads = [(a, e) for a, e, k in rows if k == 'PAD']
    # an execution region's size is rounded up to 4: 1 to 3 bytes at its end are armlink's too
    ext = [(int(mm.group(2), 16), int(mm.group(2), 16) + int(mm.group(3), 16)) for mm in
           re.finditer(r'Execution Region (\w+) \(Base: 0x([0-9a-fA-F]+), Size: 0x([0-9a-fA-F]+)', listing)]
    tails = [(a, e) for a, e in holes if e - a < 4 and any(x <= a and e == y for x, y in ext)]
    holes = [h for h in holes if h not in tails]
    if holes:
        log('FAIL flash bytes that no section and no PAD row covers: ' + ', '.join(f'0x{a:08X}-0x{e:08X}' for a, e in holes[:8])); ok = False
    log(f'listing: 0x{lay.base:08X}-0x{lay.table[1]:08X} is sections, {len(pads)} alignment PAD rows ({sum(e - a for a, e in pads)} bytes) '
        f'and {len(tails)} region end{"s" if len(tails) != 1 else ""} rounded up to 4 ({", ".join(f"0x{a:08X}+{e - a}" for a, e in tails)}), all armlink\'s' + ('; nothing else' if not holes else ''))
    # the stand-ins: verified on their own at their addresses; here the library member is there
    for st in sorted(skip):
        p = next(p for p in proved.values() if stem(p.src) == st)
        rng = [(a, z) for s_, o, sec, a, z in lay.flash if s_ == st]
        agree = p.whole and all(full[a - lay.base:a - lay.base + z] == want[a - lay.base:a - lay.base + z] for a, z in rng)
        log(f'{rel(p.src)}: ' + ('a whole match on its own, and the library member linked at ' +
            ', '.join(f'0x{a:08X}' for a, z in rng) + ' is the same bytes' if agree else 'FAIL: it and the library member disagree'))
        if not agree: ok = False
    out_img = os.path.join(GATE, 'A319.img'); open(out_img, 'wb').write(full)
    same = full == want
    diff = [i for i in range(len(want)) if full[i] != want[i]]
    log(f'image: {rel(out_img)} (0x{lay.base:08X}-0x{lay.end:08X}): ' + ('IDENTICAL to the original image' if same else
        f'{len(diff)} bytes differ, the first at 0x{lay.base + diff[0]:08X}'))
    if not same: ok = False
    header = struct.pack('<HH', gc['version'], gc['magic'])
    out_bin = os.path.join(GATE, 'A319_ARMStore.bin'); open(out_bin, 'wb').write(header + full)
    orig_bin = open(gc['bin'], 'rb').read()
    log(f'bin: {rel(out_bin)} (header version {gc["version"]}, magic 0x{gc["magic"]:04X}): ' +
        ('IDENTICAL to ' + rel(gc['bin']) if header + full == orig_bin else 'DIFFERS from ' + rel(gc['bin'])))
    if header + full != orig_bin: ok = False
    hex_text = ehex.write(lay.base, full, like=cfg.image).encode('ascii')
    out_hex = os.path.join(GATE, 'A319.hex'); open(out_hex, 'wb').write(hex_text)
    hsame = hex_text == open(cfg.image, 'rb').read()
    log(f'hex: {rel(out_hex)}: ' + ('IDENTICAL to ' + rel(cfg.image) if hsame else 'differs from ' + rel(cfg.image)))
    if not hsame: ok = False
    nproved = sum(x[4] for x in lay.flash if x[0] in order)
    log(f'one armlink run: {nproved} bytes of proved sections, the library members, the region table and the compressed RW data; '
        f'no blob of the original\'s flash bytes')
    log(f'-- gate --whole: {"PASS" if ok else "FAIL"} in {time.time() - t0:.0f}s' + (' (--fast)' if fast else ''))
    with open(os.path.join(GATE, 'report.txt'), 'w') as f: f.write('\n'.join(report) + '\n')
    return 0 if ok else 1


class _InProcess:
    """run_links' pool for one link run here, in this process."""
    def map_async(self, fn, items, chunksize=1):
        out = [fn(x) for x in items]

        class R:
            def get(self, timeout=None): return out
        return R()


def whole_image(res, lay):
    """The flash bytes of the whole link from lay.base on: every execution region's bytes at its load
    address (fromelf --bin gives each region's contents; the RW region's file holds its compressed
    load data, which follows the region table)."""
    out = bytearray()
    bases = region_bases(res['listing'])
    loads = load_bases(res['listing'])
    for reg, b in res['bins'].items():
        a = loads.get(reg, bases.get(reg))
        if a is None or not (lay.base <= a < 0x08100000): continue
        o = a - lay.base
        if len(out) < o + len(b): out += bytes(o + len(b) - len(out))
        out[o:o + len(b)] = b
    return out


_LOADBASE = re.compile(r'Execution Region (\w+) \(Exec base: 0x([0-9a-fA-F]+), Load base: 0x([0-9a-fA-F]+)')


def load_bases(text):
    return {m.group(1): int(m.group(3), 16) for m in _LOADBASE.finditer(text)}



def coverage(log, lay, have, proved, srcs):
    total = len(have)
    n = {k: sum(1 for x in have if x == k) for k in (1, 2, 3, 4, 5, 6)}
    by_dir = {}
    for st, o, sec, a, z in lay.flash:
        if st in lay.placed: continue
        d = '/'.join(rel(next(p.src for p in proved.values() if stem(p.src) == st)).split(os.sep)[:2])   # src/<top>
        by_dir[d] = by_dir.get(d, 0) + z
    pct = lambda x: f'{100.0 * x / total:5.1f}%'
    log(f'coverage of the 0x{total:X} ({total}) bytes of the image:')
    log(f'  proved sources (C and assembly)        {n[1] + n[5]:7d}  {pct(n[1] + n[5])}')
    log(f'    linked by armlink                    {n[1]:7d}  {pct(n[1])}  ' +
        ', '.join(f'{d} {z}' for d, z in sorted(by_dir.items())))
    if n[5]:
        placed = sorted(rel(p.src) for p in proved.values() if stem(p.src) in lay.placed)
        log(f'    proved by object placement           {n[5]:7d}  {pct(n[5])}  ' +
            (', '.join(placed) if len(placed) <= 4 else f'{len(placed)} sources'))
    if n[6]:
        log(f'  alignment padding (linker)             {n[6]:7d}  {pct(n[6])}')
    log(f'  C library members (linked from c_w.l, fz_ws.l) {n[2]:7d}  {pct(n[2])}')
    log(f'  region table + compressed RW (armlink) {n[3]:7d}  {pct(n[3])}')
    log(f'  original bytes (blobs)                 {n[4]:7d}  {pct(n[4])}')
    with open(os.path.join(GATE, 'coverage.tsv'), 'w') as f:
        f.write('start\tend\tkind\n')
        kinds = {1: 'C', 2: 'library', 3: 'linker', 4: 'blob', 5: 'placed', 6: 'padding', 0: 'none'}
        i = 0
        while i < total:
            j = i
            while j < total and have[j] == have[i]: j += 1
            f.write(f'{lay.base + i:08X}\t{lay.base + j:08X}\t{kinds[have[i]]}\n'); i = j


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
