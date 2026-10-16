"""Fetch the firmware images the gates compare with, check them and put them in place.

    python3 tools/fetch_firmware.py [--zip ID=PATH ...] [--only ID ...] [--force]

For every package in firmware/manifest.toml: download it (url, then fallback_url), or read a
local copy given with --zip ID=PATH; check each member named there against its sha256 and write
it to firmware/<dest>; then make the derived files (the A316 Intel HEX file) and check those too.
A file already in place with the right sha256 is left alone unless --force.

Nothing is written outside firmware/, and nothing is written that fails its check. Python 3.11
or later; no other packages. Exit status 0 when every file is in place and checked.
"""
import hashlib, io, os, sys, tomllib, urllib.request, zipfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STORE = os.path.join(REPO, 'firmware')


def sha256(b):
    return hashlib.sha256(b).hexdigest()


def download(url):
    req = urllib.request.Request(url, headers={'User-Agent': 'fetch_firmware.py'})
    with urllib.request.urlopen(req, timeout=120) as r:
        return r.read()


def ihex(data, base, entry=None, width=16, end='\r\n'):
    """Intel HEX text: data records of width bytes, an extended linear address record (type 04)
    wherever the upper 16 bits change, a start linear address record (type 05) and the end."""
    def rec(kind, addr, payload):
        b = bytes([len(payload), (addr >> 8) & 0xFF, addr & 0xFF, kind]) + payload
        return ':' + b.hex().upper() + '%02X' % (-sum(b) & 0xFF)
    lines, upper, off = [], None, 0
    while off < len(data):
        a = base + off
        n = min(width, len(data) - off, 0x10000 - (a & 0xFFFF))
        if a >> 16 != upper:
            upper = a >> 16
            lines.append(rec(4, 0, upper.to_bytes(2, 'big')))
        lines.append(rec(0, a & 0xFFFF, data[off:off + n]))
        off += n
    if entry is not None: lines.append(rec(5, 0, entry.to_bytes(4, 'big')))
    lines.append(rec(1, 0, b''))
    return (end.join(lines) + end).encode('ascii')


def in_place(dest, want):
    p = os.path.join(STORE, dest)
    return os.path.exists(p) and sha256(open(p, 'rb').read()) == want


def write(dest, data):
    p = os.path.join(STORE, dest)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    tmp = p + '.part'
    with open(tmp, 'wb') as f: f.write(data)
    os.replace(tmp, p)


def package(pkg, local, force):
    pid, ok = pkg['id'], True
    members, derived = pkg.get('member', []), pkg.get('derived', [])
    if not force and all(in_place(m['dest'], m['sha256']) for m in members + derived):
        print(f'{pid}: in place')
        return True
    if local:
        blob = open(local, 'rb').read(); where = local
    else:
        blob, where = None, None
        for u in (pkg.get('url'), pkg.get('fallback_url')):
            if not u: continue
            try:
                blob, where = download(u), u; break
            except Exception as e:          # noqa: BLE001  (network errors of every kind)
                print(f'{pid}: {u}: {e}')
        if blob is None:
            print(f'{pid}: FAIL: no download. Fetch {pkg["path"]} from {pkg["repo"]} yourself '
                  f'and pass --zip {pid}=PATH')
            return False
    got = sha256(blob)
    if pkg.get('sha256') and got != pkg['sha256']:
        print(f'{pid}: note: the package from {where} has sha256 {got}, the manifest {pkg["sha256"]};'
              ' checking its members')
    z = zipfile.ZipFile(io.BytesIO(blob))
    for m in members:
        try:
            data = z.read(m['name'])
        except KeyError:
            print(f'{pid}: FAIL: no member {m["name"]!r} in the package'); ok = False; continue
        h = sha256(data)
        if h != m['sha256']:
            print(f'{pid}: FAIL: {m["name"]}: sha256 {h}, want {m["sha256"]}'); ok = False; continue
        write(m['dest'], data)
        print(f'{pid}: {m["name"]} -> firmware/{m["dest"]} ({len(data)} bytes, sha256 ok)')
    for d in derived:
        src = os.path.join(STORE, d['from'])
        if not os.path.exists(src):
            print(f'{pid}: FAIL: {d["dest"]}: no {d["from"]}'); ok = False; continue
        data = open(src, 'rb').read()
        if d['kind'] == 'ihex':
            out = ihex(data[d.get('skip', 0):], d['base'], d.get('entry'))
        else:
            print(f'{pid}: FAIL: unknown kind {d["kind"]}'); ok = False; continue
        h = sha256(out)
        if h != d['sha256']:
            print(f'{pid}: FAIL: {d["dest"]}: sha256 {h}, want {d["sha256"]}'); ok = False; continue
        write(d['dest'], out)
        print(f'{pid}: made firmware/{d["dest"]} from {d["from"]} (sha256 ok)')
    return ok


def main(argv):
    a = list(argv)
    if '-h' in a or '--help' in a: print(__doc__); return 0
    force = '--force' in a
    if force: a.remove('--force')
    local, only = {}, set()
    while '--zip' in a:
        i = a.index('--zip'); k, _, v = a[i + 1].partition('='); local[k] = v; del a[i:i + 2]
    while '--only' in a:
        i = a.index('--only'); only.add(a[i + 1]); del a[i:i + 2]
    if a: print(__doc__); return 2
    with open(os.path.join(STORE, 'manifest.toml'), 'rb') as f:
        man = tomllib.load(f)
    ok = True
    for pkg in man.get('package', []):
        if only and pkg['id'] not in only: continue
        ok &= package(pkg, local.get(pkg['id']), force)
    print('all firmware in place' if ok else 'FAILED')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
