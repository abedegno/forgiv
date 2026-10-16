"""Apply a table of renames to the tree: src/ (C, assembly, headers, layout specs), map/functions.tsv,
map/data.tsv and toolchain.toml (and the matching work's type list, a_types.txt, when a checkout
still has it; it was removed after v1.0.0), then re-solve the .ebss order of every file that
defines its own variables.

    python3 tools/rename.py TABLE.tsv --batch functions|vars|types|fields [options]

    --hold FILE      old names (first column) not to rename (with the reason in the second)
    --only A,B       only these old names (for bisecting a batch)
    --dry-run        report what would change, write nothing
    --no-solve       do not run tools/ebss_solve.py afterwards

TABLE has the columns addr, kind, old, new, basis, note and an optional scope (empty: every
file; else a comma list of file basenames the row is limited to). Every applied row is appended
to map/names.tsv (addr, kind, old, new, basis, note).

Batches. functions: kind function. vars: kind var, and the ram_view.h (formerly a_ram.h) macros that alias a RAM word
(old var_XXXX), since the same identifier is a symbol in other files. types: kind type and the
other macros. fields: kind field.

Functions, variables, types and macros are renamed as whole words, all at once (so a -> b,
b -> c chains cannot run into each other). In assembly (and inline asm strings) `_old` becomes
`_new`; a switch table row sw_old in map/data.tsv becomes sw_new. TI's own files (F2806x_*,
CodeStartBranch.asm, f2806x_flash.h) are not touched. A macro or type defined in a .c file is
renamed in that file only. A name that the map did not have (sub_3FFEBB, ram_CDE0: they resolved
by the address they spell) gets a map/data.tsv row with that address, so that it still resolves;
a scoped var row that is not the symbol's main name (a struct view of a scalar) gets an alias row.

Fields are renamed by struct: old is STRUCT.field (or STRUCT.member.field for a nested struct, or
VAR.field for an anonymous struct declared with VAR). The member is renamed in the struct's
definition(s); the uses are found by the compiler: every source is compiled with the native
cl2000, and each `struct "S" has no field "f"` error points at a line whose `.f` or `->f` (or a
macro used on it whose body has one) takes the new name, kept only if the errors go down.
Temporary unique names are used until the end, so swaps and chains are safe."""
import argparse, collections, csv, glob, os, re, shutil, subprocess, sys, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d316

ROOT = d316.ROOT
TI = re.compile(r'^(F2806x_|f2806x_|DSP28|CodeStartBranch)')
VAR_ALIAS = re.compile(r'^var_[0-9A-Fa-f]{4}$')
WORD = re.compile(r'[A-Za-z_]\w*')


def files():
    out = []
    for p in sorted(glob.glob(os.path.join(ROOT, 'src', '**', '*'), recursive=True)):
        if os.path.isfile(p) and p.endswith(('.c', '.h', '.asm', '.spec')) and not TI.match(os.path.basename(p)):
            out.append(p)
    out.append(os.path.join(ROOT, 'toolchain.toml'))
    # the matching work's type list (removed after v1.0.0): updated only when a checkout has it
    types = os.path.join(ROOT, 'tools', 'climb', 'a_types.txt')
    if os.path.isfile(types):
        out.append(types)
    return out


def load(table, batch, hold, only):
    rows = []
    with open(table) as f:
        for r in csv.DictReader(f, delimiter='\t'):
            r['scope'] = r.get('scope') or ''
            k = r['kind']
            b = 'functions' if k == 'function' else 'fields' if k == 'field' else \
                'vars' if k == 'var' or (k == 'macro' and VAR_ALIAS.match(r['old'])) else 'types'
            if b != batch or r['old'] in hold or (only and r['old'] not in only): continue
            if r['old'] == r['new']: continue
            rows.append(r)
    return rows


def defined_in_c_only(name, texts):
    """the .c files that define macro or type `name`, if no header defines or uses it"""
    pat = re.compile(r'(#\s*define\s+%s\b|\b(struct|union|enum)\s+%s\s*\{|\}\s*%s\s*;)' % ((re.escape(name),) * 3))
    where = [p for p, t in texts.items() if pat.search(t)]
    word = re.compile(r'\b%s\b' % re.escape(name))
    in_header = any(p.endswith('.h') and word.search(t) for p, t in texts.items())
    if where and all(p.endswith('.c') for p in where) and not in_header:
        return {os.path.basename(p) for p in where} | \
               {os.path.splitext(os.path.basename(p))[0] + '.spec' for p in where}
    return None


# ---- whole-word renames ------------------------------------------------------------------------
def word_renames(rows, dry):
    paths = files()
    texts = {p: open(p, encoding='latin1').read() for p in paths}
    glob_map, scoped = {}, collections.defaultdict(dict)
    for r in rows:
        scope = r['scope']
        if not scope and r['kind'] in ('type', 'macro') and not VAR_ALIAS.match(r['old']):
            only = defined_in_c_only(r['old'], texts)
            if only: scope = ','.join(sorted(only))
        if scope:
            for f in scope.split(','): scoped[f][r['old']] = r['new']
        else:
            glob_map[r['old']] = r['new']
    syms = {r['old'] for r in rows if r['kind'] in ('function', 'var') or VAR_ALIAS.match(r['old'])}
    changed = collections.Counter()
    for p in paths:
        base = os.path.basename(p)
        m = dict(glob_map); m.update(scoped.get(base, {}))
        def sub(mo):
            w = mo.group(0)
            if w in m: return m[w]
            if w.startswith('_') and w[1:] in m and w[1:] in syms: return '_' + m[w[1:]]
            return w
        t = texts[p]
        if p.endswith('.spec'):
            t = spec_names(t, m)
        n = WORD.sub(sub, t)
        if n != texts[p]:
            changed[os.path.relpath(p, ROOT)] += 1
            if not dry: open(p, 'w', encoding='latin1').write(n)
    return changed, glob_map, scoped


def spec_names(t, m):
    """a layout spec line `ADDR DECL` names var_ADDR implicitly: give it its new name"""
    out = []
    for line in t.split('\n'):
        mo = re.match(r'([0-9A-Fa-f]{4,6})\s+(.*?)\s*(?:#\s*(\w+))?$', line)
        if mo and not line.startswith('#') and not mo.group(3):
            v = 'var_%04X' % int(mo.group(1), 16)
            if v in m: line = line.rstrip() + '   # ' + v
        out.append(line)
    return '\n'.join(out)


def map_renames(rows, glob_map, dry):
    """functions.tsv and data.tsv: names (and sw_ rows), new rows for names that resolved only by
    the address they spelled, alias rows for views"""
    fpath, dpath = os.path.join(ROOT, 'map', 'functions.tsv'), os.path.join(ROOT, 'map', 'data.tsv')
    syms = {r['old']: r for r in rows if not r['scope'] and (r['kind'] in ('function', 'var') or VAR_ALIAS.match(r['old']))}
    have = set()
    out = {}
    for path in (fpath, dpath):
        lines = open(path).read().split('\n')
        for i, line in enumerate(lines):
            f = line.split('\t')
            if len(f) < 3 or f[0] == 'addr': continue
            n = f[2]
            if n in syms: f[2] = syms[n]['new']
            elif n.startswith('sw_') and n[3:] in syms: f[2] = 'sw_' + syms[n[3:]]['new']
            if len(f) > 4:
                f[4] = WORD.sub(lambda mo: glob_map.get(mo.group(0), mo.group(0)), f[4])
            have.add(f[2])
            lines[i] = '\t'.join(f)
        out[path] = lines
    added = []
    for old, r in syms.items():
        if r['new'] in have: continue
        a = d316._HEXNAME.match(old)
        if not a: continue
        addr = int(a.group(1), 16)
        kind = 'rom' if r['kind'] == 'function' else 'var'
        added.append('%06X\t\t%s\t%s\tname for the address %s spelled; %s' % (addr, r['new'], kind, old, r['note'][:80]))
        have.add(r['new'])
    for r in rows:
        if r['scope'] and r['kind'] == 'var' and r['new'] not in have and r['addr']:
            addr = int(r['addr'], 16)
            main = glob_map.get(r['old'], r['old'])
            added.append('%06X\t\t%s\talias\tview of %s (%s) in %s' % (addr, r['new'], main, r['old'], r['scope']))
            have.add(r['new'])
    if added:
        out[dpath] = [l for l in out[dpath] if l != ''] + added + ['']
    if not dry:
        for path, lines in out.items(): open(path, 'w').write('\n'.join(lines))
    return added


# ---- fields ------------------------------------------------------------------------------------
_STRIP = {}


def strip_comments(t):
    if t in _STRIP: return _STRIP[t]
    _STRIP[t] = _strip(t)
    return _STRIP[t]


def _strip(t):
    return re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: re.sub(r'[^\n]', ' ', m.group(0)), t, flags=re.S)


def struct_bodies(text):
    """[(kind, tag, declarators after the body, start of body, end of body)] for every struct or
    union body in text (comments blanked), nested ones too"""
    t = strip_comments(text)
    out = []
    for m in re.finditer(r'\b(struct|union)\s*(\w*)\s*\{', t):
        i, depth = m.end(), 1
        while depth and i < len(t):
            if t[i] == '{': depth += 1
            elif t[i] == '}': depth -= 1
            i += 1
        after = re.match(r'\s*([^;{}]*)', t[i:])
        names = WORD.findall(after.group(1)) if after else []
        out.append((m.group(1), m.group(2), names, m.end(), i - 1))
    return out


def top_level_member(text, lo, hi, field):
    """positions of `field` declared at the top level of the body text[lo:hi]"""
    t = strip_comments(text)
    pos, depth = [], 0
    for mo in re.finditer(r'[{}]|\b%s\b' % re.escape(field), t[lo:hi]):
        s = mo.group(0)
        if s == '{': depth += 1
        elif s == '}': depth -= 1
        elif depth == 0: pos.append(lo + mo.start())
    return pos


_BODIES = {}


def find_definitions(texts, key, heads=None):
    """[(path, position)] of the member key names (STRUCT.field, STRUCT.sub.field, VAR.field);
    heads: the names STRUCT or VAR has now (an earlier batch may have renamed it)"""
    parts = key.split('.')
    head, path, field = parts[0], parts[1:-1], parts[-1]
    heads = heads or {head}
    hits = []
    for p, text in texts.items():
        if (p, text) not in _BODIES: _BODIES[(p, text)] = struct_bodies(text)
        bodies = _BODIES[(p, text)]
        roots = [b for b in bodies if b[1] in heads or heads & set(b[2])]
        for b in roots:
            lo, hi = b[3], b[4]
            for sub in path:          # a nested member: the body inside whose declarator is sub
                inner = [c for c in bodies if lo <= c[3] and c[4] <= hi and sub in c[2]]
                if not inner: lo = hi = None; break
                lo, hi = inner[0][3], inner[0][4]
            if lo is None: continue
            for q in top_level_member(text, lo, hi, field): hits.append((p, q))
    return hits


class Native:
    def __init__(self):
        self.cl = d316.native('cl2000')
        self.inc = os.path.join(os.path.dirname(d316.cfg()['native']['bin']), 'include')
        self.tmp = tempfile.mkdtemp(prefix='rename_')

    def errors(self, src):
        import match
        p = subprocess.run([self.cl, '-v28', '-ml', '-mt', '--float_support=fpu32', '--abi=coffabi', '-c', '--set_error_limit=100000',
                            '--obj_directory=' + self.tmp, '--asm_directory=' + self.tmp,
                            '-I' + os.path.join(ROOT, 'src', 'include'), '-I' + self.inc] + match.cflags_of(src) + [src],
                           capture_output=True, text=True, cwd=self.tmp)
        out = p.stdout + p.stderr
        errs = re.findall(r'"([^"]+)", line (\d+): error[^:]*: (struct|union) "([^"]+)" has no field "(\w+)"', out)
        other = [l for l in out.split('\n') if ': error' in l and 'has no field' not in l]
        return [(os.path.join(os.path.dirname(src), f) if not os.path.isabs(f) else f, int(n), s, fld) for f, n, k, s, fld in errs], other


def field_renames(rows, dry, renamed=None, say=print):
    paths = [p for p in files() if p.endswith(('.c', '.h'))]
    texts = {p: open(p, encoding='latin1').read() for p in paths}
    orig = dict(texts)
    tmpname, done, missing = {}, [], []
    # 1. definitions -> temporary names
    edits = collections.defaultdict(list)
    at = {}                                           # (path, position) -> tmp
    heads_of = collections.defaultdict(set)          # tmp -> struct names it answers to
    conflicts, found = [], []
    for k, r in enumerate(rows):
        head = r['old'].split('.')[0]
        heads = {head} | set((renamed or {}).get(head, ()))
        hits = find_definitions(texts, r['old'], heads)
        if not hits:
            missing.append(r['old']); continue
        found.append(r)
        tmp = 'RNTMP%d_%s' % (k, r['new'])
        for p, q in hits:
            if (p, q) in at:                          # the same member, reached by another row
                other = at[(p, q)]
                if tmpname[other]['new'] != r['new']:
                    conflicts.append('%s (%s) and %s (%s) name the same member in %s'
                                     % (tmpname[other]['old'], tmpname[other]['new'], r['old'], r['new'],
                                        os.path.relpath(p, ROOT)))
                heads_of[other] |= heads
                continue
            at[(p, q)] = tmp
            tmpname.setdefault(tmp, r)
            heads_of[tmp] |= heads
            edits[p].append((q, len(r['old'].split('.')[-1]), tmp))
    for c in conflicts: say('  CONFLICT: ' + c)
    for p, es in edits.items():
        t = texts[p]
        for q, n, tmp in sorted(es, reverse=True): t = t[:q] + tmp + t[q + n:]
        texts[p] = t
    by_field = collections.defaultdict(list)          # old field -> [(struct name, tmp)]
    for tmp, r in tmpname.items():
        for h in heads_of[tmp]: by_field[r['old'].split('.')[-1]].append((h, tmp))

    def flush():
        for p in paths:
            open(p, 'w', encoding='latin1').write(texts[p])

    def restore():
        for p in paths:
            open(p, 'w', encoding='latin1').write(orig[p])

    flush()
    nat = Native()
    csrc = [p for p in paths if p.endswith('.c')]
    try:
        for src in csrc:
            errs, other = nat.errors(src)
            if other:
                say('  %s: other errors: %s' % (os.path.relpath(src, ROOT), other[:3]))
            skip = set()
            guard = 0
            while guard < 5000:
                guard += 1
                todo = [e for e in errs if e not in skip]
                if not todo: break
                e0 = todo[0]
                f, line, sname, fld = e0
                cands = []
                for head, tmp in by_field.get(fld, []):
                    if (head == sname or sname == '<unnamed>') and tmp not in cands: cands.append(tmp)
                if not cands:
                    cands = list(dict.fromkeys(tmp for head, tmp in by_field.get(fld, [])))
                sites = member_sites(texts, f, line, fld, paths)
                fixed = False
                for (p, q) in sites:
                    for tmp in cands:
                        t0 = texts[p]
                        texts[p] = t0[:q] + tmp + t0[q + len(fld):]
                        open(p, 'w', encoding='latin1').write(texts[p])
                        e2, o2 = nat.errors(src)
                        b4, af = collections.Counter(errs), collections.Counter(e2)
                        # this error went (a member inside it may now show, on the same line)
                        if af[e0] < b4[e0] and len(o2) <= len(other) and \
                                not any(x[3].startswith('RNTMP') for x in e2) and \
                                all(af[x] <= b4[x] for x in af if (x[0], x[1]) != (e0[0], e0[1])):
                            errs, other, fixed = e2, o2, True
                            break
                        texts[p] = t0
                        open(p, 'w', encoding='latin1').write(t0)
                    if fixed: break
                if not fixed:
                    say('  %s:%d: cannot place field %s of %s' % (os.path.relpath(f, ROOT), line, fld, sname))
                    skip.add(e0)
        # 2. temporary names -> new names
        for p in paths:
            texts[p] = re.sub(r'\bRNTMP\d+_(\w+)', r'\1', texts[p])
        if dry: restore()
        else: flush()
    except BaseException:
        restore(); raise
    return found, missing


def member_sites(texts, f, line, fld, paths):
    """positions of `.fld` / `->fld` on line `line` of file f, else in the bodies of the macros
    used on that line"""
    f = os.path.abspath(f)
    t = texts.get(f)
    if t is None: return []
    starts = [0] + [m.end() for m in re.finditer('\n', t)]
    lo = starts[line - 1]; hi = starts[line] if line < len(starts) else len(t)
    pat = re.compile(r'(\.|->)\s*(%s)\b' % re.escape(fld))
    sites = [(f, lo + m.start(2)) for m in pat.finditer(t[lo:hi])]
    if sites: return sites
    words = set(WORD.findall(t[lo:hi]))
    for p in paths:
        for m in re.finditer(r'(?m)^[ \t]*#\s*define\s+(\w+)(\([^)]*\))?[^\n]*(\\\n[^\n]*)*', texts[p]):
            if m.group(1) in words:
                body = m.group(0)
                for s in pat.finditer(body): sites.append((p, m.start() + s.start(2)))
    return sites


# ---- main --------------------------------------------------------------------------------------
def log(rows):
    path = os.path.join(ROOT, 'map', 'names.tsv')
    new = not os.path.exists(path)
    with open(path, 'a') as f:
        if new: f.write('addr\tkind\told\tnew\tbasis\tnote\n')
        for r in rows:
            f.write('\t'.join((r['addr'], r['kind'], r['old'], r['new'], r['basis'], r['note'])) + '\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('table')
    ap.add_argument('--batch', required=True, choices=('functions', 'vars', 'types', 'fields'))
    ap.add_argument('--hold')
    ap.add_argument('--only')
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--no-solve', action='store_true')
    a = ap.parse_args()
    hold = set()
    if a.hold and os.path.exists(a.hold):
        for line in open(a.hold):
            if line.strip() and not line.startswith('#'): hold.add(line.split('\t')[0].strip())
    only = set(a.only.split(',')) if a.only else None
    rows = load(a.table, a.batch, hold, only)
    print('%s: %d rows' % (a.batch, len(rows)))
    if a.batch == 'fields':
        # the struct and variable names the earlier batches gave (any scope)
        renamed = collections.defaultdict(set)
        with open(a.table) as f:
            for r in csv.DictReader(f, delimiter='\t'):
                if r['kind'] != 'field' and r['old'] not in hold: renamed[r['old']].add(r['new'])
        applied, missing = field_renames(rows, a.dry_run, renamed)
        if missing: print('  no definition found for: %s' % ' '.join(missing))
    else:
        changed, glob_map, scoped = word_renames(rows, a.dry_run)
        added = map_renames(rows, glob_map, a.dry_run)
        print('  %d files changed; %d map rows added' % (len(changed), len(added)))
        for x in added: print('   +', x)
        applied = rows
    if a.dry_run: return
    log(applied)
    print('  logged %d renames to map/names.tsv' % len(applied))
    if not a.no_solve:
        r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'ebss_solve.py'), '--all'])
        if r.returncode: print('ebss_solve: some file is not solved'); sys.exit(1)


if __name__ == '__main__':
    main()
