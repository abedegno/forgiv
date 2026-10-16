"""Check that the map covers every word of the image once, and print a summary.

    python3 tools/coverage.py

Every word of the image must be in exactly one row of map/functions.tsv or map/data.tsv (flash
rows), or be 0xFFFF fill outside every row. Ramfunc rows are counted at their load address.
Prints functions by kind, words by kind, and any overlap, gap or uncovered non-fill word; exits
1 if there is one."""
import collections, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d316


def coverage():
    owner = {}
    problems = []
    rows = [('f', r) for r in d316.functions()] + [('d', r) for r in d316.data()]
    for t, r in rows:
        if not d316.BASE <= r['addr'] < d316.END: continue
        if r['size'] <= 0:
            problems.append('%06X %s: no size' % (r['addr'], r['name'])); continue
        for a in range(r['addr'], r['addr'] + r['size']):
            if a >= d316.END:
                problems.append('%06X %s: runs past the image' % (r['addr'], r['name'])); break
            if a in owner:
                problems.append('%06X: in %s and %s' % (a, owner[a][1]['name'], r['name'])); break
            owner[a] = (t, r)
    words = collections.Counter()
    for a in range(d316.BASE, d316.END):
        if a in owner:
            t, r = owner[a]
            words[('code ' if t == 'f' else 'data ') + r['kind']] += 1
        elif d316.W(a) == 0xFFFF:
            words['fill'] += 1
        else:
            problems.append('%06X: word %04X in no row' % (a, d316.W(a)))
            words['uncovered'] += 1
    return words, problems


def main():
    words, problems = coverage()
    fk = collections.Counter(r['kind'] for r in d316.functions())
    dk = collections.Counter(r['kind'] for r in d316.data())
    print('functions: %d  (%s)' % (sum(fk.values()), ', '.join('%s %d' % kv for kv in sorted(fk.items()))))
    print('data rows: %d  (%s)' % (sum(dk.values()), ', '.join('%s %d' % kv for kv in sorted(dk.items()))))
    print('words of the image (65536):')
    for k in sorted(words, key=lambda k: (k.split()[0], -words[k])):
        print('  %-16s %6d  %5.1f%%' % (k, words[k], 100.0 * words[k] / 65536))
    for p in problems[:40]: print('PROBLEM', p)
    if len(problems) > 40: print('... %d problems' % len(problems))
    print('COVERED' if not problems else 'NOT COVERED: %d problems' % len(problems))
    sys.exit(1 if problems else 0)


if __name__ == '__main__':
    main()
