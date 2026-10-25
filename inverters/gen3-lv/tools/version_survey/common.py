"""Paths and binary access for the local, non-accepting version survey."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[4]
WORK = ROOT / 'build/gen3-lv-reloc'
WORK.mkdir(parents=True, exist_ok=True)
PC_RELATIVE = re.compile(r'\[pc, #(-?0x[0-9a-f]+|-?\d+)\]')


class ArmImage:
    def __init__(self, version):
        self.base = 0x08014000
        self.data = (ROOT / f'firmware/gen3-lv/A{version}_ARMStore.bin').read_bytes()[4:]
        self.end = self.base + len(self.data)

    def bytes(self, address, length):
        offset = address - self.base
        if offset < 0 or offset + length > len(self.data):
            raise ValueError('read outside firmware payload')
        return self.data[offset:offset + length]
