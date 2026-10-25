"""Check A319 map bounds, source ownership, vectors and optional gate coverage."""
import argparse
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = 0x08014000


def rows(name):
    with (ROOT / 'map' / name).open() as stream:
        return list(csv.DictReader(stream, delimiter='\t'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--coverage', type=Path)
    args = parser.parse_args()
    binary = (ROOT.parents[2] / 'firmware/gen3-lv/A319_ARMStore.bin').read_bytes()
    assert binary[:4] == bytes.fromhex('3f016655'), 'wrong ARMStore header'
    image = binary[4:]
    end = BASE + len(image)
    functions, data = rows('functions.tsv'), rows('data.tsv')
    names = set()
    for row in functions + data:
        assert row['name'] not in names, f"duplicate symbol: {row['name']}"
        names.add(row['name'])
        address, size = int(row['addr'], 16), int(row['size'], 16)
        if 0x08000000 <= address < 0x09000000:
            assert BASE <= address <= address + size <= end, row
        if row.get('file'):
            assert (ROOT / row['file']).exists(), f"missing source: {row['file']}"
    previous_end = BASE
    for row in sorted(functions, key=lambda item: int(item['addr'], 16)):
        address, size = int(row['addr'], 16), int(row['size'], 16)
        assert address % 2 == 0 and size > 0, row
        assert address >= previous_end, f"overlapping function: {row['name']}"
        previous_end = address + size
    vectors = [int.from_bytes(image[i:i + 4], 'little') for i in range(0, 0x150, 4)]
    assert vectors[0] == 0x2000A7D8, 'unexpected initial stack pointer'
    for index, value in enumerate(vectors[1:], 1):
        assert value == 0 or (value & 1 and BASE <= (value & ~1) < end), (index, value)
    coverage_message = ''
    if args.coverage:
        with args.coverage.open() as stream:
            coverage = list(csv.DictReader(stream, delimiter='\t'))
        cursor = BASE
        for row in coverage:
            start, stop = int(row['start'], 16), int(row['end'], 16)
            assert start == cursor and stop > start, f'gap or overlap at {cursor:08X}'
            assert row['kind'] in {'C', 'library', 'linker', 'placed', 'padding'}, row
            cursor = stop
        assert cursor == end, 'coverage does not reach the image end'
        coverage_message = f'; {len(image)} flash bytes covered, zero original-byte blobs'
    print(f'Map valid: {len(functions)} functions, {len(data)} data symbols{coverage_message}')


if __name__ == '__main__':
    main()
