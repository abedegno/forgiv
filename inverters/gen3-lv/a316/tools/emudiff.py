"""Run functions of a mod build and of the original side by side in an emulator and compare what
they do: Exhume's differential tester (profiles/armcc5/tools/emu/difftest.py, Unicorn, Cortex-M)
with this tree's settings. Functions are matched by name, so the mod build may be laid out
anywhere; its ELF gives its addresses.

    $EXHUME/.venv/bin/python tools/emudiff.py [--image AXF] [--runs N] [--all] [--mutate FUNC=OFF:BYTE] [FUNC ...]

--image AXF   the mod build's ELF (default build/mod/ARMStore.axf, which tools/modbuild.py writes);
              its symbols for the tester go beside it (ARMStore.syms.tsv: functions and constants
              from the ELF, RAM names from map/data.tsv)
--all         every function the C sources define, not only the pure ones below
FUNC ...      these functions instead
--runs N      inputs per function (default 200)
--mutate      the tester's self-check: one byte of the mod build changed at FUNC+OFF

The default list is the pure functions: the Modbus CRC (which reads its tables in .constdata),
the XOR checksum, the integer helpers of util.c, isqrt_div and counter_wrap_inc, and the fault
bit helpers (RAM in, RAM out). Exit status 1 when any function behaves differently."""
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import a316env  # noqa: E402

PURE = ['crc16_modbus', 'calc_checksum_xor_index', 'min3', 'max3', 'median3', 'clamp', 'div_u16_safe',
        'isqrt_div', 'counter_wrap_inc', 'fault_set', 'fault_clear']


def symbols(image):
    """The mod build's symbols for the tester, as a TSV beside the image: its functions and
    constants from the ELF (where the build put them), and the RAM names from map/data.tsv. The
    sources reach RAM through the map's names, which armlink takes from a symdefs file and keeps
    as absolute symbols the tester does not read as data; tools/modbuild.py checks that the RAM is
    where the map says (check 2), so those addresses hold in the mod build too."""
    sys.path.insert(0, os.path.join(a316env.EXHUME, 'profiles', 'armcc5', 'tools', 'emu'))
    import images
    raw = open(image, 'rb').read()
    rows = {}
    for name, value, size, kind in images.elf_symbols(raw):
        if kind == 'func': rows[name] = (value & ~1, size, 'code')
        elif kind == 'object': rows[name] = (value, size, 'data')
    for name, (a, n, kind) in images._map_rows(os.path.join(ROOT, 'map', 'data.tsv')):
        # RAM: the map's row (the ELF has the symdefs' names with no size); a constant the ELF
        # has with no size: the map's size at the ELF's address
        if a >= 0x20000000: rows[name] = (a, n, 'data')
        elif name in rows and rows[name][2] == 'data' and not rows[name][1]: rows[name] = (rows[name][0], n, 'data')
    out = os.path.splitext(image)[0] + '.syms.tsv'
    with open(out, 'w') as f:
        f.write('name\taddr\tsize\tkind\n')
        for name, (a, n, k) in sorted(rows.items()): f.write(f'{name}\t{a:08X}\t{n:X}\t{k}\n')
    return out


def main(argv):
    image = os.path.join(ROOT, 'build', 'mod', 'ARMStore.axf')
    runs, funcs, rest, every = '200', [], [], False
    i = 0
    while i < len(argv):
        x = argv[i]
        if x == '--image': image = argv[i + 1]; i += 2; continue
        if x == '--runs': runs = argv[i + 1]; i += 2; continue
        if x == '--mutate': rest += [x, argv[i + 1]]; i += 2; continue
        if x == '--all': every = True; i += 1; continue
        funcs.append(x); i += 1
    if not os.path.exists(image): sys.exit(f'emudiff: no {image} (run tools/modbuild.py first)')
    tool = os.path.join(a316env.EXHUME, 'profiles', 'armcc5', 'tools', 'emu', 'difftest.py')
    syms = symbols(image)
    cmd = [sys.executable, tool, '--config', os.path.join(ROOT, 'exhume.toml'), '--b', image, '--b-syms', syms,
           '--runs', runs] + rest + ([] if every else (funcs or PURE))
    print('emudiff: the original (A316.hex, map/) against ' + os.path.relpath(image, ROOT), flush=True)
    return subprocess.run(cmd, cwd=ROOT).returncode


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
