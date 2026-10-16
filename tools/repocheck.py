"""Repository checks that need neither the toolchains nor the firmware images, for CI.

    python3 tools/repocheck.py [--root DIR]

CI cannot run the gates: the compilers (Keil, TI) and the images must never be on GitHub. So this
is what CI runs (.github/workflows/check.yml), and what to run before a pull request besides the
gates. The idea is Exhume's tools/repocheck.py, adapted to firmware.

Tracked means committed or not ignored, so a new file is checked before it is added.

- Python: every tracked .py compiles; every .sh passes `sh -n`.
- Markdown links: every relative link in a tracked .md file names a file or directory in the
  repository, and an #anchor into a Markdown file names a heading there as GitHub makes them
  (links in code spans and code blocks are ignored; web links are not fetched).
- No binary files: none by extension (images, packages, objects, executables, libraries), and
  none by content: a NUL byte, text that is not UTF-8, a known binary header (ELF, MZ, zip, 7z,
  TI COFF, the ARMStore.bin header) or Intel HEX records.
- Text hygiene: tracked text is ASCII and names no home directory (a macOS or Linux home path).
- The manifest: firmware/manifest.toml parses, each package has its fields, each member and
  derived file a 64-digit sha256 and a dest inside firmware/, no dest twice, and each tree it
  names exists.

Exit status 0 when all pass.
"""
import os, re, subprocess, sys, tomllib

BANNED = ['bin', 'hex', 'srec', 's19', 'zip', '7z', 'rar', 'gz', 'tgz', 'xz', 'bz2', 'exe', 'dll',
          'so', 'dylib', 'com', 'obj', 'o', 'a', 'lib', 'l', 'out', 'axf', 'elf', 'coff', 'img',
          'iso', 'dfu', 'chm', 'pdf', 'jar', 'class', 'pyc']
HEADERS = [(b'\x7fELF', 'an ELF file'), (b'MZ', 'a DOS or Windows executable'),
           (b'PK\x03\x04', 'a zip archive'), (b'7z\xbc\xaf', 'a 7z archive'),
           (b'\x1f\x8b', 'a gzip file'), (b'\xc2\x00', 'a TI COFF object'),
           (b'\x93\x00', 'a TI COFF object'), (b'\x3c\x01\x66\x55', 'an ARMStore.bin image')]
HEX_RECORD = re.compile(rb'^:[0-9A-Fa-f]{10,}\r?$', re.M)
HOME = re.compile('/' + r'(?:Users|home)/[A-Za-z0-9_.-]+')     # split so this file does not match
SHA = re.compile(r'^[0-9a-f]{64}$')


def tracked(root):
    r = subprocess.run(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'],
                       cwd=root, capture_output=True, text=True, check=True)
    return sorted(p for p in r.stdout.split('\0') if p and os.path.isfile(os.path.join(root, p))
                  and not os.path.islink(os.path.join(root, p)))


def check_code(root, files, problems):
    for p in files:
        full = os.path.join(root, p)
        if p.endswith('.py'):
            try: compile(open(full, 'rb').read(), p, 'exec')
            except SyntaxError as e: problems.append(f'{p}: line {e.lineno}: {e.msg}')
        elif p.endswith('.sh'):
            r = subprocess.run(['sh', '-n', full], capture_output=True, text=True)
            if r.returncode: problems.append(f'{p}: sh -n: {r.stderr.strip()}')


def md_anchors(full, cache={}):
    """The anchors GitHub gives a Markdown file's headings (lower case, punctuation dropped,
    spaces to hyphens, -1, -2 ... for repeats), and any <a name> or <a id>."""
    if full not in cache:
        found, seen, code = set(), {}, False
        for line in open(full, encoding='utf-8', errors='replace'):
            if line.lstrip().startswith('```'): code = not code
            if code: continue
            m = re.match(r'#{1,6}\s+(.*?)\s*#*\s*$', line)
            if m:
                h = re.sub(r'\]\([^)]*\)', '', re.sub(r'<[^>]+>', '', m.group(1))).lower()
                a = re.sub(r'[^\w\- ]', '', h).replace(' ', '-')
                n = seen.get(a, 0); seen[a] = n + 1
                found.add(a if n == 0 else f'{a}-{n}')
            found.update(re.findall(r'<a\s+(?:name|id)="([^"]+)"', line))
        cache[full] = found
    return cache[full]


def check_links(root, files, problems):
    for p in files:
        if not p.lower().endswith('.md'): continue
        text = open(os.path.join(root, p), encoding='utf-8', errors='replace').read()
        text = re.sub(r'(?ms)^\s*```.*?^\s*```', '', text)   # code blocks
        text = re.sub(r'`[^`\n]*`', '', text)                 # code spans
        for target in re.findall(r'\]\(([^)\s]+)(?:\s+"[^"]*")?\)', text):
            if re.match(r'[a-z][a-z0-9+.-]*:', target, re.I): continue
            path, _, anchor = target.partition('#')
            full = os.path.normpath(os.path.join(root, os.path.dirname(p), path)) if path else os.path.join(root, p)
            if os.path.commonpath([os.path.abspath(full), os.path.abspath(root)]) != os.path.abspath(root):
                problems.append(f'{p}: link to {target}, outside the repository'); continue
            if not os.path.exists(full):
                problems.append(f'{p}: link to {target}, which does not exist')
            elif anchor and full.lower().endswith('.md') and not re.fullmatch(r'L\d+(-L\d+)?', anchor) \
                    and anchor.lower() not in md_anchors(full):
                problems.append(f'{p}: link to {target}, which has no such heading')


def check_payload(root, files, problems):
    ext = re.compile(r'\.(' + '|'.join(re.escape(e) for e in BANNED) + r')$', re.I)
    for p in files:
        if ext.search(p):
            problems.append(f'{p}: a binary file type this repository must not hold'); continue
        data = open(os.path.join(root, p), 'rb').read()
        kind = next((what for head, what in HEADERS if data.startswith(head)), None)
        if kind:
            problems.append(f'{p}: looks like {kind}'); continue
        if b'\0' in data:
            problems.append(f'{p}: binary (has NUL bytes)'); continue
        try:
            text = data.decode('utf-8')
        except UnicodeDecodeError:
            problems.append(f'{p}: binary (not UTF-8 text)'); continue
        if len(HEX_RECORD.findall(data)) >= 8:
            problems.append(f'{p}: holds Intel HEX records (a firmware image?)'); continue
        bad = next((i for i, c in enumerate(text) if ord(c) > 127), None)
        if bad is not None:
            line = text.count('\n', 0, bad) + 1
            problems.append(f'{p}: line {line}: not ASCII ({text[bad]!r})')
        m = HOME.search(text)
        if m:
            line = text.count('\n', 0, m.start()) + 1
            problems.append(f'{p}: line {line}: a home directory path ({m.group(0)})')


def check_manifest(root, problems):
    path = os.path.join(root, 'firmware', 'manifest.toml')
    if not os.path.exists(path):
        problems.append('firmware/manifest.toml: missing'); return
    try:
        man = tomllib.load(open(path, 'rb'))
    except tomllib.TOMLDecodeError as e:
        problems.append(f'firmware/manifest.toml: {e}'); return
    store = os.path.abspath(os.path.join(root, 'firmware'))
    dests, ids = {}, set()
    pkgs = man.get('package', [])
    if not pkgs: problems.append('firmware/manifest.toml: no [[package]]')
    for i, pkg in enumerate(pkgs):
        where = f'firmware/manifest.toml: package {pkg.get("id", i)}'
        for k in ('id', 'repo', 'path', 'url', 'firmware'):
            if not pkg.get(k): problems.append(f'{where}: no {k}')
        if pkg.get('id') in ids: problems.append(f'{where}: id used twice')
        ids.add(pkg.get('id'))
        for u in (pkg.get('url'), pkg.get('fallback_url')):
            if u and not u.startswith('https://'): problems.append(f'{where}: {u} is not https')
        if pkg.get('sha256') and not SHA.match(pkg['sha256']): problems.append(f'{where}: bad package sha256')
        for t in pkg.get('firmware', []):
            if not os.path.isdir(os.path.join(root, t)): problems.append(f'{where}: no tree {t}')
        members = pkg.get('member', [])
        if not members: problems.append(f'{where}: no [[package.member]]')
        produced = set()
        for kind, rows, need in (('member', members, ('name', 'sha256', 'dest')),
                                 ('derived', pkg.get('derived', []), ('kind', 'from', 'sha256', 'dest'))):
            for r in rows:
                w = f'{where}: {kind} {r.get("name") or r.get("dest")}'
                for k in need:
                    if not r.get(k): problems.append(f'{w}: no {k}')
                if r.get('sha256') and not SHA.match(r['sha256']): problems.append(f'{w}: bad sha256')
                d = r.get('dest')
                if not d: continue
                full = os.path.abspath(os.path.join(store, d))
                if os.path.isabs(d) or os.path.commonpath([full, store]) != store or full == store:
                    problems.append(f'{w}: dest {d} is not inside firmware/')
                if d in dests: problems.append(f'{w}: dest {d} also used by {dests[d]}')
                dests[d] = w
                if kind == 'derived':
                    if r.get('kind') != 'ihex': problems.append(f'{w}: unknown kind {r.get("kind")}')
                    if r.get('from') not in produced: problems.append(f'{w}: from {r.get("from")} is no member\'s dest')
                    for k in ('base', 'skip', 'entry'):
                        if k in r and not isinstance(r[k], int): problems.append(f'{w}: {k} is not a number')
                produced.add(d)


def main(argv):
    a = list(argv)
    roots = []
    while '--root' in a:
        i = a.index('--root'); roots.append(a[i + 1]); del a[i:i + 2]
    if a: print(__doc__); return 2
    root = roots[0] if roots else subprocess.run(['git', 'rev-parse', '--show-toplevel'], capture_output=True,
                                                 text=True, check=True).stdout.strip()
    problems = []
    files = tracked(root)
    check_code(root, files, problems)
    check_links(root, files, problems)
    check_payload(root, files, problems)
    check_manifest(root, problems)
    for x in problems: print('PROBLEM', x)
    print(f'{len(files)} tracked files: ' + ('all checks pass' if not problems else f'{len(problems)} problems'))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
