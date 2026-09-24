#!/usr/bin/env python3
"""List member names in the Gameloft GLA archives used by N.O.V.A. 2."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


def xor32_decode(data: bytes, seed: int = 0x3857A) -> bytes:
    """Decode the name-table transform used by this build's CCustomPakReader."""
    state = seed

    def rand(limit: int) -> int:
        nonlocal state
        state = (state * 0x19660D + 0x3C6EF35F) & 0xFFFFFFFF
        return ((state >> 16) % limit) if limit else (state >> 16)

    out = bytearray(data)

    def ror(value: int, bits: int, width: int) -> int:
        mask = (1 << width) - 1
        bits %= width
        return ((value >> bits) | (value << (width - bits))) & mask

    pos = 0
    while len(out) - pos >= 4:
        direction = rand(2)
        shift = rand(32)
        value = int.from_bytes(out[pos : pos + 4], "little")
        value = ror(value, shift if direction else 32 - shift, 32)
        key = [rand(256) for _ in range(4)]
        mask = key[3] | (key[2] << 8) | (key[1] << 16) | (key[0] << 24)
        out[pos : pos + 4] = (value ^ mask).to_bytes(4, "little")
        pos += 4

    remain = len(out) - pos
    if remain:
        direction = rand(2)
        if remain == 1:
            shift = rand(8)
            value = out[pos]
            value = ror(value, shift if direction else 8 - shift, 8)
            out[pos] = value ^ rand(256)
        elif remain == 2:
            shift = rand(16)
            value = int.from_bytes(out[pos : pos + 2], "little")
            value = ror(value, shift if direction else 16 - shift, 16)
            mask = (rand(256) << 8) | rand(256)
            out[pos : pos + 2] = (value ^ mask).to_bytes(2, "little")
        else:
            shift = rand(24)
            value = int.from_bytes(out[pos : pos + 3], "little")
            value = ror(value, shift if direction else 24 - shift, 24)
            mask = (rand(256) << 16) | (rand(256) << 8) | rand(256)
            out[pos : pos + 3] = (value ^ mask).to_bytes(3, "little")
    return bytes(out)


def list_members(path: Path) -> list[str]:
    data = path.read_bytes()
    if len(data) < 16:
        raise ValueError("file is shorter than the GLA header")

    data_end, names_end, names_start, count = struct.unpack(">4I", data[:16])
    if not count or count > 100_000:
        raise ValueError(f"invalid member count: {count}")
    index_start = 16
    names_base = index_start + count * 16
    if names_start != names_base or names_end < names_start or names_end > len(data):
        raise ValueError(
            f"invalid index offsets: names start={names_start}, "
            f"expected={names_base}, end={names_end}, file size={len(data)}"
        )

    names = bytearray(data[names_start:names_end])
    offsets = []
    for index in range(count):
        record = data[index_start + index * 16 : index_start + (index + 1) * 16]
        offsets.append(struct.unpack(">4I", record)[2])

    members = []
    previous = None
    for offset in offsets:
        if offset >= len(names) or (previous is not None and offset <= previous):
            raise ValueError(f"invalid/non-monotonic name offset: {offset}")
        if previous is not None:
            names[previous : offset - 1] = xor32_decode(names[previous : offset - 1])
        previous = offset
    assert previous is not None
    names[previous : len(names) - 1] = xor32_decode(names[previous : len(names) - 1])
    for offset in offsets:
        end = names.find(0, offset)
        if end < 0:
            raise ValueError(f"unterminated member name at {offset}")
        members.append(names[offset:end].decode("utf-8", "replace"))
    return members


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="backslashreplace")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="a .gla archive")
    args = parser.parse_args()
    for name in list_members(args.archive):
        print(name)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
