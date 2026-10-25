"""Find the runtime library's members in the image.

    python3 tools/libfind.py [--lib PATH] [--all]

Extracts rts2800_fpu32.lib of the configured compiler into build/rts/<version>/ and searches the
image for every member's code and constant sections, with the words that relocations change
masked. A section of fewer than 4 words is reported only where it sits in the runtime library's
part of .text (the image's own code has many one-word LRETR). Prints one line per placed section:

    ADDR  SIZE  MEMBER  SECTION  SYMBOLS(offset)

and, with --tsv, the rows mkmap.py takes (one per global symbol in a placed code section)."""
import argparse, glob, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d319

RTS_AREA = (0x3F14DE, 0x3F1C54)     # where the image's library code sits (with some application code)


def extract(lib=None):
    lib = lib or os.path.join(d319.cfg()['compiler']['cgt'], d319.cfg()['compiler']['rtslib'])
    ver = os.path.basename(os.path.dirname(os.path.dirname(lib)))
    out = os.path.join(d319.BUILD, 'rts', ver)
    if not os.path.isdir(out) or not glob.glob(os.path.join(out, '*.obj')):
        os.makedirs(out, exist_ok=True)
        d319.run([d319.native('ar2000'), '-x', os.path.abspath(lib)], cwd=out)
    return lib, out


def search(sec, img, index):
    ws, bits = sec.data, d319.reloc_masks(sec)
    mask = [b == 0xFFFF for b in bits]
    n = len(ws)
    if n == 0: return []
    anchor = next((k for k in range(n - 1) if not bits[k] and not bits[k + 1]), None)
    if anchor is None:
        cands = range(len(img) - n + 1)
    else:
        cands = [p - anchor for p in index.get((ws[anchor], ws[anchor + 1]), [])]
    hits = []
    for s in cands:
        if s < 0 or s + n > len(img): continue
        if all((img[s + k] ^ ws[k]) & ~bits[k] & 0xFFFF == 0 for k in range(n)): hits.append(d319.BASE + s)
    return hits


def find(lib=None):
    lib, d = extract(lib)
    img = d319.image()
    index = {}
    for i in range(len(img) - 1): index.setdefault((img[i], img[i + 1]), []).append(i)
    found, objs, later = [], {}, []
    for path in sorted(glob.glob(os.path.join(d, '*.obj'))):
        o = objs[path] = d319.Obj(path)
        syms = [s for s in o.symbols if s['sclass'] in ('C_EXT', 'C_STAT') and s['kind'] == 'defined'
                and not s['name'].startswith(('$', '.')) and s['section']]
        for sec in o.sections.values():
            if sec.kind not in ('text', 'data') or not sec.data: continue
            if sec.name.startswith('.cinit'): continue
            hits = search(sec, img, index)
            if len(sec.data) < 4 or sec.kind == 'data':
                hits = [h for h in hits if RTS_AREA[0] <= h < RTS_AREA[1] or (sec.kind == 'data' and len(sec.data) >= 8)
                        or sec.kind == 'data' and len(hits) < 20]
            if len(hits) != 1:
                if sec.kind == 'data' and hits: later.append((path, sec, hits))
                continue
            ss = sorted((s['value'], s['name']) for s in syms if s['section'] == sec.name)
            found.append(dict(addr=hits[0], size=len(sec.data), member=os.path.basename(path), section=sec.name,
                              kind=sec.kind, syms=ss))
    # a small constant section with several hits: the one next to its member's other constants
    for path, sec, hits in later:
        m = os.path.basename(path)
        near = [h for h in hits for f in found if f['member'] == m and f['kind'] == 'data'
                and -32 <= h - f['addr'] <= f['size'] + 32]
        if len(near) == 1:
            found.append(dict(addr=near[0], size=len(sec.data), member=m, section=sec.name, kind='data', syms=[]))
    # a section too small to place by its words alone is placed where a placed member calls it
    # (boot.obj's LCR __system_post_cinit names the one-word startup.obj)
    defs = {}
    for path, o in objs.items():
        for name, (secname, off) in o.defined().items():
            defs.setdefault(name, (path, secname, off))
    placed = {(f['member'], f['section']) for f in found}
    changed = True
    while changed:
        changed = False
        for f in list(found):
            sec = objs[os.path.join(d, f['member'])].sections[f['section']]
            for off, typ, sym in sec.relocs:
                if typ != 'R_REL22' or sym not in defs: continue
                at = f['addr'] + off
                tgt = ((d319.W(at) & 0x3F) << 16) | d319.W(at + 1)
                path, secname, v = defs[sym]
                key = (os.path.basename(path), secname)
                if key in placed: continue
                s2 = objs[path].sections[secname]
                if s2.kind != 'text' or not d319.BASE <= tgt - v < d319.END: continue
                if all((d319.W(tgt - v + k) ^ s2.data[k]) & ~b & 0xFFFF == 0 for k, b in enumerate(d319.reloc_masks(s2))):
                    o = objs[path]
                    ss = sorted((y['value'], y['name']) for y in o.symbols if y['section'] == secname and
                                y['sclass'] in ('C_EXT', 'C_STAT') and y['kind'] == 'defined' and not y['name'].startswith(('$', '.')))
                    found.append(dict(addr=tgt - v, size=len(s2.data), member=key[0], section=secname, kind='text', syms=ss,
                                      how=f'called from {f["member"]}'))
                    placed.add(key); changed = True
    # constants only of members whose code is in the image
    code = {f['member'] for f in found if f['kind'] == 'text'}
    found = [f for f in found if f['kind'] == 'text' or f['member'] in code]
    return lib, sorted(found, key=lambda f: f['addr'])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lib')
    a = ap.parse_args()
    lib, found = find(a.lib)
    print(f'# {lib}')
    for f in found:
        print('%06X %5d  %-16s %-14s %s' % (f['addr'], f['size'], f['member'], f['section'],
                                            ' '.join('%s(+%d)' % (n, v) for v, n in f['syms'])))
    print(f'# {len(found)} sections, {sum(f["size"] for f in found)} words')


if __name__ == '__main__':
    main()
