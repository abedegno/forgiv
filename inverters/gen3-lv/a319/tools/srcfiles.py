"""Where the sources are. No tool names a source directory: every source is found by walking
src/, by its stem (unique across src/, which gate.py checks), or by an address its at: lines name.

    sources(tree)      every .c and .s under tree (default src/), not tree/include
    decomp(tree)       the decompiled C: every .c under tree but tree/include and tree/stdperiph
                       (whose .c files only include ST's release sources)
    by_stem(stem)      the source with that stem, or None
    at_index()         {address: source} for every name an at: line places
    source_at(addr)    the source whose at: lines place addr, or None
    map_col(path)      the map's file column for a source: its path from the repo root (src/...)
    is_decomp_col(col) whether a map file column names a decompiled C source (src/... or the
                       older auto/... form)"""
import os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'src')


def sources(tree=SRC, exts=('.c', '.s')):
    out = []
    inc = os.path.join(tree, 'include')
    for d, dirs, files in os.walk(tree):
        if os.path.commonpath([d, inc]) == inc: dirs[:] = []; continue
        dirs.sort()
        out += [os.path.join(d, n) for n in sorted(files) if n.endswith(exts)]
    return out


def decomp(tree=SRC):
    sp = os.path.join(tree, 'stdperiph')
    return [p for p in sources(tree, ('.c',)) if os.path.commonpath([p, sp]) != sp]


def stem(p):
    return os.path.splitext(os.path.basename(p))[0]


def by_stem(s, tree=SRC):
    for p in sources(tree):
        if stem(p) == s: return p
    return None


_AT = re.compile(r'(?:/\*|;)\s*at:([^\n]*)')
_PAIR = re.compile(r'([A-Za-z_$][\w$.]*)\s+0x([0-9A-Fa-f]+)')


def at_index(tree=SRC):
    out = {}
    for p in sources(tree):
        for m in _AT.finditer(open(p, encoding='latin1').read()):
            for _, a in _PAIR.findall(m.group(1)): out.setdefault(int(a, 16), p)
    return out


def source_at(addr, tree=SRC):
    return at_index(tree).get(addr)


def map_col(path):
    return os.path.relpath(path, ROOT) if path else ''


def is_decomp_col(col):
    return bool(col) and col.endswith('.c') and (col.startswith('src/') or col.startswith('auto/'))
