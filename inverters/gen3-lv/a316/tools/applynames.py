"""Apply name proposals to the tree: functions (sub_ADDR) and globals (g_ADDR, v_ADDR, c_ADDR).

    python3 tools/applynames.py [-n] PROPOSALS.tsv ...

Each proposal file has columns addr, old, new, basis (original|likely|descriptive), note, with a
header row. Several files may propose the same address: the strongest basis wins, then the first
file; the rest are reported. A global's g_ and v_ spellings (the same address, declared as a byte
array or with its own type in different files) both become the new name; no file declares both.
A plain alias `#define DAT_addr NAME` is folded into its file (its uses become NAME); the typed
overlays `#define DAT_addr (*(T *)(NAME + 0))` stay until the struct step.

Renamed: every .c/.h/.s under src/, and the name column of map/functions.tsv and map/data.tsv.
Logged: one row per name in map/names.tsv (addr, kind, old, new, basis, note). Names are only
names here: the image is laid out by file order and the at: lines, so the gate must stay
IDENTICAL; run it after."""
import os, re, sys, glob, keyword
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RANK = {'original': 0, 'likely': 1, 'descriptive': 2}
C_KEYWORDS = set('''auto break case char const continue default do double else enum extern float for goto
if inline int long register restrict return short signed sizeof static struct switch typedef union
unsigned void volatile while _Bool __asm __inline __irq __packed __weak'''.split())
IDENT = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')


def read_map(name):
    rows = [l.rstrip('\n').split('\t') for l in open(os.path.join(ROOT, 'map', name))]
    return rows[0], rows[1:]


def main(argv):
    dry = '-n' in argv
    files = [a for a in argv if a != '-n']
    _, frows = read_map('functions.tsv')
    _, drows = read_map('data.tsv')
    names = {r[2]: int(r[0], 16) for r in frows} | {r[2]: int(r[0], 16) for r in drows}
    best, notes = {}, []
    for path in files:
        for k, l in enumerate(open(path)):
            f = l.rstrip('\n').split('\t')
            if k == 0 or len(f) < 4 or not f[0].strip(): continue
            addr, old, new, basis = int(f[0], 16), f[1], f[2], f[3]
            note = f[4] if len(f) > 4 else ''
            if not IDENT.match(new) or new in C_KEYWORDS:
                notes.append(f'skip {old} -> {new!r}: not a usable C identifier'); continue
            if old not in names or names[old] != addr:
                notes.append(f'skip {old} -> {new}: {old} is not the map name at 0x{addr:08X}'); continue
            cand = (RANK.get(basis, 3), old, new, basis, note, path)
            if old in best and best[old][2] != new:
                keep = min(best[old], cand)
                lose = cand if keep is best[old] else best[old]
                notes.append(f'{old}: kept {keep[2]} ({keep[3]}, {os.path.basename(keep[5])}), not {lose[2]} ({lose[3]}, {os.path.basename(lose[5])})')
                best[old] = keep
            else:
                best.setdefault(old, cand)
    # every identifier the sources already use (comments left out): a new name must not be one,
    # or a local, field or macro of that name would capture the renamed uses
    used, at_addr = set(), {}
    for path in glob.glob(os.path.join(ROOT, 'src', '**', '*.[chs]'), recursive=True):
        text = re.sub(r'/\*.*?\*/|//[^\n]*|;[^\n]*' if path.endswith('.s') else r'/\*.*?\*/|//[^\n]*', ' ',
                      open(path, encoding='latin1').read(), flags=re.S)
        used |= set(re.findall(r'\b[A-Za-z_]\w*\b', text))
        # at: lines name objects at addresses: such a name already is the object, not a clash
        for line in re.findall(r'(?:/\*|;) at: ([^\n]*)', open(path, encoding='latin1').read()):
            for n, a in re.findall(r'([A-Za-z_]\w*) 0x([0-9A-Fa-f]{8})', line): at_addr.setdefault(n, set()).add(int(a, 16))
    # one new name per old, unique, and not another address's existing name
    by_new = {}
    for old, c in sorted(best.items(), key=lambda kv: kv[1][0]):
        new = c[2]
        if new in by_new or (new in names and names[new] != names[old]):
            notes.append(f'skip {old} -> {new}: the name is taken'); continue
        if new in used and at_addr.get(new) != {names[old]}:
            notes.append(f'skip {old} -> {new}: already an identifier in the sources'); continue
        by_new[new] = old
    pairs = {old: best[old] for old in by_new.values()}
    # aliases: v_ADDR beside g_ADDR
    subst = {}
    for old, c in pairs.items():
        subst[old] = c[2]
        if old.startswith('g_'): subst['v_' + old[2:]] = c[2]
    pat = re.compile(r'\b(' + '|'.join(map(re.escape, sorted(subst, key=len, reverse=True))) + r')\b') if subst else None
    alias = re.compile(r'^#define (DAT_[0-9a-fA-F]{8}) ([A-Za-z_]\w*)\n', re.M)
    changed = 0
    for path in glob.glob(os.path.join(ROOT, 'src', '**', '*.[chs]'), recursive=True):
        text = open(path, encoding='latin1').read()
        new = pat.sub(lambda m: subst[m.group(1)], text)
        # fold plain aliases of renamed names
        for d, target in alias.findall(new):
            if target in by_new:
                new = re.sub(r'^#define ' + d + ' ' + target + r'\n', '', new, flags=re.M)
                new = re.sub(r'\b' + d + r'\b', target, new)
        if new != text:
            changed += 1
            if not dry: open(path, 'w', encoding='latin1').write(new)
    for name in ('functions.tsv', 'data.tsv'):
        head, rows = read_map(name)
        for r in rows:
            if r[2] in pairs: r[2] = pairs[r[2]][2]
        if not dry:
            with open(os.path.join(ROOT, 'map', name), 'w') as f:
                f.write('\t'.join(head) + '\n' + ''.join('\t'.join(r) + '\n' for r in rows))
    if not dry:
        log = os.path.join(ROOT, 'map', 'names.tsv')
        fresh = not os.path.exists(log)
        with open(log, 'a') as f:
            if fresh: f.write('addr\tkind\told\tnew\tbasis\tnote\n')
            for old, c in sorted(pairs.items(), key=lambda kv: names[kv[0]]):
                kind = 'function' if old.startswith('sub_') else 'data'
                f.write(f'{names[old]:08X}\t{kind}\t{old}\t{c[2]}\t{c[3]}\t{c[4]}\n')
    for n in notes: print(n)
    print(f'{"would rename" if dry else "renamed"} {len(pairs)} names in {changed} source files')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
