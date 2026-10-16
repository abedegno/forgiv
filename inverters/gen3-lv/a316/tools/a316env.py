"""Where the A316 tools find what is not in this repository: the Exhume toolkit, Keil's
compiler homes and Wine. Every tool imports this first.

Each setting is taken from the first of: an environment variable, toolchain.local.toml in this
directory (gitignored; copy toolchain.example.toml), the default.

    EXHUME        an Exhume checkout (github.com/abedegno/Exhume).
                  Default: the first of ../exhume and ../Exhume beside the repository, or an
                  exhume/ directory at the repository root.
    MDK_TOOLCHAIN a directory of Keil MDK homes, one per release, named MDKnnn (MDK516, MDK521 ...),
                  each holding ARM/ARMCC and ARM/sw. Default: toolchain/ in this directory (a
                  directory or a symbolic link; gitignored). Homes are taken from it by name.
    MDK_HOME      the compile home, MDK 5.21 or later (armcc 5.06).
                  Default: MDK521 in MDK_TOOLCHAIN (exhume.toml's [toolchain] home).
    MDK_LIBHOME   the library home for the gate's library link, MDK 5.16 or later.
                  Default: MDK516 in MDK_TOOLCHAIN (exhume.toml's [gate] libhome).
    WINE_BIN      the directory holding wine and wineserver. Default: the one on PATH, else
                  Wine Devel's on macOS (/Applications/Wine Devel.app/Contents/Resources/wine/bin).
    MDK_LICENSED_HOME    a licensed MDK home (5.21 or later, armcc 5.06), for the modes that link
                  more than MDK-Lite's 32 KB at once: gate.py --whole and modbuild.py. Unset, they
                  skip with a message. Nothing else uses it.
    MDK_LICENSED_PREFIX  the Wine prefix that MDK was installed in: the licence check reads the
                  registry there, so its tools run in that prefix. Default: the directory above
                  drive_c in MDK_LICENSED_HOME's path.

ST's StdPeriph library V3.5.0 is not in the repository either: unpack it (or link it) at
lib/stdperiph35 in this directory (docs/development/toolchains.md at the repository root). The /* opts: */
lines of src/stdperiph name it as {root}/lib/stdperiph35, and {root} is filled in here.
"""
import os, shutil, subprocess, sys, tomllib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))     # inverters/gen3-lv/a316
REPO = os.path.dirname(os.path.dirname(os.path.dirname(ROOT)))          # the repository root
MAC_WINE = '/Applications/Wine Devel.app/Contents/Resources/wine/bin'


def _local():
    p = os.path.join(ROOT, 'toolchain.local.toml')
    if not os.path.exists(p): return {}
    with open(p, 'rb') as f: return tomllib.load(f)


LOCAL = _local()


def setting(env, key, default=None):
    v = os.environ.get(env) or LOCAL.get(key)
    if v: return os.path.abspath(os.path.join(ROOT, os.path.expanduser(str(v))))
    return default


def _exhume():
    p = setting('EXHUME', 'exhume')
    if p: return p
    for c in (os.path.join(REPO, '..', 'exhume'), os.path.join(REPO, '..', 'Exhume'), os.path.join(REPO, 'exhume')):
        if os.path.isdir(os.path.join(c, 'profiles', 'armcc5')): return os.path.abspath(c)
    return None


EXHUME = _exhume()
if not EXHUME or not os.path.isdir(os.path.join(EXHUME, 'profiles', 'armcc5', 'tools')):
    sys.exit('A316 tools: no Exhume checkout found. Set EXHUME to one (git clone '
             'https://github.com/abedegno/Exhume), or set exhume in toolchain.local.toml.')
TOOLCHAIN = setting('MDK_TOOLCHAIN', 'mdk_toolchain', os.path.join(ROOT, 'toolchain'))
MDK_HOME = setting('MDK_HOME', 'mdk_home')            # None: exhume.toml's [toolchain] home
MDK_LIBHOME = setting('MDK_LIBHOME', 'mdk_libhome')   # None: exhume.toml's [gate] libhome


LICENSED_HOME = setting('MDK_LICENSED_HOME', 'mdk_licensed_home')


def _licensed_prefix():
    p = setting('MDK_LICENSED_PREFIX', 'mdk_licensed_prefix')
    if p or not LICENSED_HOME: return p
    parts = LICENSED_HOME.split(os.sep)
    return os.sep.join(parts[:parts.index('drive_c')]) if 'drive_c' in parts else None


LICENSED_PREFIX = _licensed_prefix()


def _wine():
    b = setting('WINE_BIN', 'wine_bin')
    if b: return b
    w = shutil.which('wine')
    if w: return os.path.dirname(w)
    return MAC_WINE if os.path.isdir(MAC_WINE) else None


WINE_BIN = _wine()
if WINE_BIN and WINE_BIN not in os.environ.get('PATH', '').split(os.pathsep):
    os.environ['PATH'] = WINE_BIN + os.pathsep + os.environ.get('PATH', '')
os.environ.setdefault('EXHUME_ARMCC_RUNNER', 'wine')
os.environ.setdefault('WINEDEBUG', '-all')

for _p in (os.path.join(EXHUME, 'profiles', 'armcc5', 'tools'), os.path.join(EXHUME, 'tools')):
    if _p not in sys.path: sys.path.insert(0, _p)

import config  # noqa: E402  (Exhume's)

_directives = config.Config.directives


def _expand(self, src):
    """Exhume's /* target: */ and /* opts: */ reader, with {root} in the opts made this
    directory, so that no source holds a machine's path."""
    t, o = _directives(self, src)
    return t, (o.replace('{root}', ROOT) if o else o)


if not getattr(config.Config.directives, '_a316', False):
    _expand._a316 = True
    config.Config.directives = _expand


# ---- the licensed MDK ---------------------------------------------------------------------------
# Its tools run with no ARM_TOOL_VARIANT (so the licence's own variant is used, not mdk_lite) and in
# the prefix MDK was installed in. What they print about the licence is never passed on: a line
# that names the licence, its feature or its server is dropped (keep_line).
_LIC = ('LIC', 'Checkout feature', 'Feature version', 'LICENSE_FILE', 'ARM_TOOL_VARIANT', 'ARM_PRODUCT_PATH',
        'Product location', 'Toolchain location', 'Selected tool variant', 'license', 'License', 'LICENSE')


def keep_line(l):
    """False for a line of tool output that says anything about the licence (L9555E, the error
    that a check failed, is kept)."""
    return 'L9555E' in l or not any(k in l for k in _LIC)


def scrub(text):
    out = [l for l in text.splitlines() if keep_line(l)]
    if len(out) != len(text.splitlines()): out.append('(licence details withheld)')
    return '\n'.join(out)


def licensed(home):
    """True when home is the licensed MDK home."""
    return bool(LICENSED_HOME and home and os.path.realpath(home) == os.path.realpath(LICENSED_HOME))


def require_licensed(what):
    """The licensed home, or exit 0 with a message saying the mode was skipped."""
    if not LICENSED_HOME:
        print(f'{what}: SKIPPED: it needs a licensed MDK (more than MDK-Lite\'s 32 KB in one link). '
              'Set MDK_LICENSED_HOME, or mdk_licensed_home in toolchain.local.toml, to its home '
              '(the directory holding ARM/ARMCC).', flush=True)
        sys.exit(0)
    if not os.path.isfile(os.path.join(LICENSED_HOME, 'ARM', 'ARMCC', 'bin', 'armlink.exe')):
        sys.exit(f'{what}: {LICENSED_HOME} has no ARM/ARMCC/bin/armlink.exe')
    if not LICENSED_PREFIX or not os.path.isfile(os.path.join(LICENSED_PREFIX, 'system.reg')):
        sys.exit(f'{what}: no Wine prefix for the licensed MDK (set MDK_LICENSED_PREFIX)')
    return LICENSED_HOME


def _wine_tool(name):
    w = shutil.which(name)
    return w or (os.path.join(WINE_BIN, name) if WINE_BIN else name)


def start_licensed():
    """A persistent wineserver in the licensed prefix, with Wine's services started by a program
    that has no output pipes (else the first captured tool call never returns; docs/gate.md, Wine)."""
    env = dict(os.environ, WINEPREFIX=LICENSED_PREFIX, WINEDEBUG='-all')
    q = dict(env=env, start_new_session=True, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run([_wine_tool('wineserver'), '-p'], timeout=120, **q)
    subprocess.run([_wine_tool('wine'), 'cmd', '/c', 'exit'], timeout=300, **q)


def stop_licensed():
    if LICENSED_PREFIX:
        subprocess.run([_wine_tool('wineserver'), '-k'], env=dict(os.environ, WINEPREFIX=LICENSED_PREFIX),
                       capture_output=True)


def _patch_toolchain():
    """armtc.Toolchain.run, with the licensed home's tools run the licensed way (above)."""
    import armtc
    if getattr(armtc.Toolchain.run, '_a316', False): return
    plain = armtc.Toolchain.run

    def run(self, tool, args, cwd):
        if not (self.runner == 'wine' and licensed(self.home)): return plain(self, tool, args, cwd)
        env = dict(os.environ, WINEDEBUG='-all', WINEPREFIX=LICENSED_PREFIX)
        for k in ('ARM_TOOL_VARIANT', 'ARM_PRODUCT_PATH', 'ARMLMD_LICENSE_FILE', 'LM_LICENSE_FILE'): env.pop(k, None)
        cmd = ['wine', os.path.join(self.home, 'ARM', 'ARMCC', 'bin', f'{tool}.exe')] + args
        for attempt in range(3):
            r = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd, env=env)
            out = scrub(armtc.clean(r.stdout + r.stderr))
            if r.returncode in (0, 1): return r.returncode, out
        return r.returncode, out
    run._a316 = True
    armtc.Toolchain.run = run


_patch_toolchain()


def load():
    """exhume.toml, with the compiler and library homes from the settings above."""
    cfg = config.load(os.path.join(ROOT, 'exhume.toml'))
    g = cfg.raw.setdefault('gate', {})
    cfg.home = MDK_HOME or os.path.join(TOOLCHAIN, os.path.basename(cfg.home))
    g['libhome'] = MDK_LIBHOME or os.path.join(TOOLCHAIN, os.path.basename(g.get('libhome', 'MDK516')))
    return cfg
