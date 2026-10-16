"""A fail test for a gate or a mod build: change one constant in a copy of a tree's sources, run
the tool on the copy, and require that it fails.

    python3 tools/ci/failtest.py TREE FILE OLD NEW -- COMMAND ...

TREE is a firmware tree (inverters/gen3-lv/a316), FILE a source under its src/, and OLD a piece
of text that occurs exactly once in FILE. The whole of src/ is copied to TREE/build/failtest/src
(so that the copy's relative includes resolve), OLD is replaced by NEW in the copy of FILE, and
COMMAND runs in TREE with {copy} replaced by the changed copy of FILE and {src} by the copied
src/. src/ itself is never changed.

Exit status 0 when COMMAND exits non-zero and prints a line with FAIL (the change was caught),
1 otherwise. The last lines of COMMAND's output are printed either way, and the copy is removed.
"""
import os, shutil, subprocess, sys


def main(argv):
    if '--' not in argv or argv.index('--') != 4:
        print(__doc__); return 2
    tree, rel, old, new = argv[:4]
    cmd = argv[5:]
    tree = os.path.abspath(tree)
    src = os.path.join(tree, 'src')
    work = os.path.join(tree, 'build', 'failtest')
    shutil.rmtree(work, ignore_errors=True)
    shutil.copytree(src, os.path.join(work, 'src'))
    rel = os.path.relpath(os.path.join(tree, rel), tree)
    if not rel.startswith('src' + os.sep): sys.exit(f'failtest: {rel} is not under src/')
    copy = os.path.join(work, rel)
    text = open(copy, encoding='latin1').read()
    n = text.count(old)
    if n != 1: sys.exit(f'failtest: {old!r} occurs {n} times in {rel}, not once')
    open(copy, 'w', encoding='latin1', newline='').write(text.replace(old, new))
    cmd = [c.replace('{copy}', copy).replace('{src}', os.path.join(work, 'src')) for c in cmd]
    print(f'failtest: {rel}: {old!r} -> {new!r}; running {" ".join(os.path.relpath(c, tree) if c.startswith(work) else c for c in cmd)}',
          flush=True)
    r = subprocess.run(cmd, cwd=tree, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace')
    lines = r.stdout.rstrip('\n').splitlines()
    for line in lines[-12:]: print('    ' + line)
    shutil.rmtree(work, ignore_errors=True)
    caught = r.returncode != 0 and any('FAIL' in line for line in lines)
    print(f'failtest: exit status {r.returncode}: ' + ('FAIL as required (the change was caught)' if caught else
                                                       'NOT CAUGHT: the tool passed a changed source'))
    return 0 if caught else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
