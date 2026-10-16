"""Disassemble a word range of the D316 image with TI's dis2000.

    python3 tools/dis.py START [END | +COUNT]     # word addresses, hex; END is exclusive
    python3 tools/dis.py NAME                     # a function or data row from the map
    python3 tools/dis.py ... --run ADDR           # place the words at ADDR (ramfuncs run in RAM)
    python3 tools/dis.py ... --raw                # dis2000's own text, without map names

The words are written as .word into a placeholder object, linked at the address and read back
with dis2000 -i, so dis2000 sees exactly the image's words. Call and branch targets that the map
names are shown with their names; ramfuncs are shown at their run address unless --load."""
import os, sys
# this file's name hides the standard library's dis module: take tools/ off the front of the path
# while the standard library loads, then put it at the end for d316
_tools = os.path.dirname(os.path.abspath(__file__))
sys.path[:] = [p for p in sys.path if os.path.abspath(p or '.') != _tools]
import argparse, inspect, re   # noqa: E402,F401  (inspect loads the real dis)
sys.path.append(_tools)
import d316                    # noqa: E402


def label_map():
    names = {}
    for r in d316.functions():
        names.setdefault(d316.run_addr(r), r['name'])
    for r in d316.data():
        names.setdefault(r['addr'], r['name'])
    return names


def annotate(text, names):
    if not re.match(r'^(LCR|LB|LC|FFC|MOVL XAR\d, #)', text): return text
    def sub(m):
        a = int(m.group(0), 16)
        return m.group(0) + (' <%s>' % names[a] if a in names else '')
    return re.sub(r'0x[0-9a-f]{4,6}\b', sub, text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('start')
    ap.add_argument('end', nargs='?')
    ap.add_argument('--run', help='disassemble as if the words were at this address')
    ap.add_argument('--load', action='store_true', help='show a ramfunc at its load address')
    ap.add_argument('--raw', action='store_true')
    a = ap.parse_args()
    run = None
    try:
        start = int(a.start, 16)
        if a.end is None: n = 0x40
        elif a.end.startswith('+'): n = int(a.end[1:], 16)
        else: n = int(a.end, 16) - start
    except ValueError:
        rows = [r for r in d316.functions() + d316.data() if r['name'] == a.start]
        if not rows: sys.exit(f'{a.start}: not in the map')
        r = rows[0]
        start, n = r['addr'], r['size'] or 0x40
        if d316.run_addr(r) != r['addr'] and not a.load: run = d316.run_addr(r)
    if a.run: run = int(a.run, 16)
    if not (d316.BASE <= start and start + n <= d316.END and n > 0): sys.exit('range outside the image')
    at = run if run is not None else start
    names = {} if a.raw else label_map()
    for addr, ws, text in d316.disasm(d316.words(start, n), at):
        if addr in names and not a.raw: print(f'{names[addr]}:')
        load = '' if run is None else '%06x ' % (addr - at + start)
        print(f'{load}{addr:06x}  {d316.fmt_words(ws):<10} {annotate(text, names)}')


if __name__ == '__main__':
    main()
