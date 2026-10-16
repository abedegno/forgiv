"""Reorder a source file's variable definitions so that the compiler lays out its .ebss as the
image has it. Works for any source under src/ (a path, or a file name or stem).

    python3 tools/ebss_solve.py SRC.c [SRC.c ...]     # check, and solve where needed
    python3 tools/ebss_solve.py --check SRC.c ...     # only report
    python3 tools/ebss_solve.py --all                 # every source that defines .ebss variables

Why. cl2000 (15.12 and 16.9.6) does not keep source order in a file's .ebss. It sorts the
variables by class (one-word scalars, two-word scalars, aggregates) with a sort that is not
stable, so inside a class the order is a permutation of the definition order. The permutation
depends on the slots (position and kind of each definition) and on the other symbols the file
declares, and identifier spellings take part too (docs/notes.md, "a callee's name can move a
file's .ebss"). So after any rename the definitions may have to be reordered.

How. The definitions sit between a begin and an end marker, one declaration per item. Three
marker styles are in the sources, and all are accepted:

    /* ebss: tools/climb/ebss.py */ ... /* end ebss */
    /* .ebss begin */ ... /* .ebss end */
    // ebss-begin ... // ebss-end   or  /* ebss-begin */ ...

They are historical marker text from the matching work, which wrote them with its own solvers
(removed after v1.0.0; they are in the v1.0.0 tag). The first one names such a solver, but it is
only text now: this tool reads it and nothing else does.

Each round compiles the real file with its own compiler and flags (under Wine, as the gate
does), reads where each variable landed, and applies the inverse permutation: the item in the
slot that received the k-th variable by address gets the k-th variable by address. A variable's
address is its map/data.tsv row, or the address its name spells (var_D8E2, ram_CDC0). The file
is right when every variable's address minus its offset gives the same section base. When the
plain iteration cycles (the spellings moved under it), the solver swaps pairs of misplaced items
and keeps a swap that lowers the number of misplaced variables.

The file is rewritten only when a solution is found (the original is kept otherwise).
Exit status 0 when every file given is right at the end."""
import argparse, os, random, re, shutil, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d316, match

BEGIN = re.compile(r'^\s*(/\*\s*(ebss: tools/climb/ebss\.py|\.ebss begin|ebss-begin)\s*\*/|//\s*ebss-begin)\s*$')
END = re.compile(r'^\s*(/\*\s*(end ebss|\.ebss end|ebss-end)\s*\*/|//\s*ebss-end)\s*$')


def strip_comments(s):
    return re.sub(r'/\*.*?\*/|//[^\n]*', ' ', s, flags=re.S)


def split_items(lines):
    """the block's lines as items: [(text lines, declared names)]; comment and blank lines go
    with the declaration after them, a trailing comment with the one before"""
    items, cur, depth = [], [], 0
    for line in lines:
        cur.append(line)
        code = strip_comments('\n'.join(cur))
        if not code.strip():
            continue
        depth = code.count('{') - code.count('}')
        if depth == 0 and code.rstrip().endswith(';'):
            items.append(cur); cur = []
    tail = cur
    out = []
    for it in items:
        code = strip_comments('\n'.join(it))
        code = re.sub(r'=\s*\{.*?\}\s*(?=[,;])', '', code, flags=re.S)       # initialiser lists
        code = re.sub(r'=[^,;]*', '', code)                                   # scalar initialisers
        code = re.sub(r'\{[^{}]*\}', ' ', code, flags=re.S)                   # struct bodies
        code = re.sub(r'\[[^\]]*\]', '', code)
        code = code.strip().rstrip(';')
        names = []
        for k, part in enumerate(code.split(',')):
            ws = re.findall(r'[A-Za-z_]\w*', part)
            if ws: names.append(ws[-1])
        out.append((it, names))
    return out, tail


def block(src):
    text = open(src, encoding='latin1').read().split('\n')
    b = next((i for i, l in enumerate(text) if BEGIN.match(l)), None)
    e = next((i for i, l in enumerate(text) if END.match(l) and b is not None and i > b), None)
    if b is None or e is None:
        return None
    items, tail = split_items(text[b + 1:e])
    return text, b, e, items, tail


def compose(text, b, e, items, tail):
    body = [l for it, _ in items for l in it] + tail
    return text[:b + 1] + body + text[e:]


_ROWS = None


def addr_of(name):
    global _ROWS
    if _ROWS is None:
        _ROWS = {r['name']: r['addr'] for r in d316.data()}
    if name in _ROWS: return _ROWS[name]
    return d316.resolve('_' + name)


def layout(w, src, work):
    """{name: offset} of the file's .ebss symbols"""
    shutil.rmtree(work, ignore_errors=True); os.makedirs(work)
    obj = d316.compile_c(w, src, work, cgt=match.cgt_of(src), extra=match.cflags_of(src))
    o = d316.Obj(obj)
    return {s['name'][1:] if s['name'].startswith('_') else s['name']: s['value'] for s in o.symbols
            if s['kind'] == 'defined' and (s.get('section') or '') == '.ebss'
            and not s['name'].startswith(('.', '$')) and s['name'] != s.get('section')}


def misplaced(lay):
    """names whose address does not fit the common base (the base most variables agree on)"""
    bases = {}
    for n, off in lay.items():
        a = addr_of(n)
        if a is None: continue
        bases.setdefault(a - off, []).append(n)
    if not bases: return [], None
    base = max(bases, key=lambda b: len(bases[b]))
    bad = [n for n in lay if addr_of(n) is None or addr_of(n) - lay[n] != base]
    return bad, base


def solve(w, src, check_only=False, rounds=40, say=print):
    rel = os.path.relpath(src, d316.ROOT)
    blk = block(src)
    work = os.path.join(d316.BUILD, 'ebss_solve', os.path.splitext(os.path.basename(src))[0])
    orig = open(src, encoding='latin1').read()
    lay = layout(w, src, work)
    bad, base = misplaced(lay)
    if not lay:
        say('%s: no .ebss variables' % rel); return True
    if not bad:
        say('%s: right (%d variables, .ebss at 0x%04X)' % (rel, len(lay), base)); return True
    say('%s: %d of %d variables misplaced' % (rel, len(bad), len(lay)))
    if check_only: return False
    if blk is None:
        say('  no ebss markers in the file: cannot reorder'); return False
    text, b, e, items, tail = blk
    inblock = {n for _, ns in items for n in ns}
    outside = [n for n in lay if n not in inblock]
    if outside:
        say('  defined outside the block (left in place): %s' % ' '.join(outside))
    multi = [ns for _, ns in items if len(ns) > 1]
    if multi:
        say('  items with several declarators move as one: %s' % '; '.join(','.join(m) for m in multi))

    def write(its):
        open(src, 'w', encoding='latin1').write('\n'.join(compose(text, b, e, its, tail)))

    def score(its):
        write(its)
        l = layout(w, src, work)
        bd, bs = misplaced(l)
        return l, bd, bs

    seen = set()
    best = (len(bad), list(items))
    for rnd in range(rounds):
        key = tuple(tuple(ns) for _, ns in items)
        if key in seen:
            break
        seen.add(key)
        # slots by output position; the k-th slot (in output order) takes the k-th item by address
        singles = [(k, ns[0]) for k, (it, ns) in enumerate(items) if len(ns) == 1 and ns[0] in lay]
        slots = sorted((lay[n], k) for k, n in singles)
        want = sorted(singles, key=lambda kn: addr_of(kn[1]) if addr_of(kn[1]) is not None else 1 << 30)
        new = list(items)
        for (off, k), (k2, n) in zip(slots, want):
            new[k] = items[k2]
        if new == items:
            break
        items = new
        lay, bad, base = score(items)
        say('  round %d: %d misplaced' % (rnd + 1, len(bad)))
        if len(bad) < best[0]: best = (len(bad), list(items))
        if not bad:
            say('%s: solved, .ebss at 0x%04X' % (rel, base)); return True
    # local search from the best order: swap two items that hold misplaced variables
    items = list(best[1])
    lay, bad, base = score(items)
    rng = random.Random(1)
    tries = 0
    while bad and tries < 400:
        tries += 1
        badk = [k for k, (it, ns) in enumerate(items) if any(n in bad for n in ns)]
        if len(badk) < 2: badk += rng.sample(range(len(items)), min(2, len(items)))
        i, j = rng.sample(badk, 2)
        trial = list(items); trial[i], trial[j] = trial[j], trial[i]
        l2, b2, bs2 = score(trial)
        if len(b2) < len(bad):
            items, lay, bad, base = trial, l2, b2, bs2
            say('  swap: %d misplaced' % len(bad))
    if not bad:
        say('%s: solved by swaps, .ebss at 0x%04X' % (rel, base)); return True
    open(src, 'w', encoding='latin1').write(orig)
    say('%s: NOT SOLVED (%d misplaced: %s); file left as it was' % (rel, len(bad), ' '.join(sorted(bad)[:12])))
    return False


def ebss_sources():
    """sources with ebss markers"""
    out = []
    for p in d316.sources(('.c',)):
        if block(p) is not None: out.append(p)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src', nargs='*')
    ap.add_argument('--all', action='store_true', help='every source with ebss markers')
    ap.add_argument('--check', action='store_true', help='report only, change nothing')
    a = ap.parse_args()
    srcs = [os.path.abspath(s if os.path.exists(s) else d316.source(s)) for s in a.src] + \
           (ebss_sources() if a.all else [])
    ok = True
    with d316.Wine() as w:
        for s in srcs:
            ok &= solve(w, s, a.check)
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
