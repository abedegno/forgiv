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

ST's StdPeriph library V3.5.0 is not in the repository either: unpack it (or link it) at
lib/stdperiph35 in this directory (docs/development/toolchains.md at the repository root). The /* opts: */
lines of src/stdperiph name it as {root}/lib/stdperiph35, and {root} is filled in here.
"""
import os, shutil, sys, tomllib

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


def load():
    """exhume.toml, with the compiler and library homes from the settings above."""
    cfg = config.load(os.path.join(ROOT, 'exhume.toml'))
    g = cfg.raw.setdefault('gate', {})
    cfg.home = MDK_HOME or os.path.join(TOOLCHAIN, os.path.basename(cfg.home))
    g['libhome'] = MDK_LIBHOME or os.path.join(TOOLCHAIN, os.path.basename(g.get('libhome', 'MDK516')))
    return cfg
