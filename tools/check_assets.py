#!/usr/bin/env python3
"""Inspect an original Elma installation without modifying or executing it."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct


def signed16(value: int) -> int:
    value &= 65535
    return value - 65536 if value >= 32768 else value


def decode_directory(raw: bytes, seed: int) -> bytes:
    result = bytearray(raw)
    a, b, c = 23, seed, 3391
    for index in range(len(result)):
        result[index] ^= a & 255
        # C/C++ signed remainder truncates toward zero, unlike Python's %.
        a -= (abs(a) // c) * c * (-1 if a < 0 else 1)
        b = signed16(b + a * c)
        a = signed16(31 * b + c)
    return bytes(result)


def find_case(directory: Path, name: str) -> Path:
    exact = directory / name
    if exact.exists():
        return exact
    found = [p for p in directory.iterdir() if p.name.lower() == name.lower()]
    if len(found) != 1:
        raise ValueError(f"Missing or ambiguous {name!r} in {directory}")
    return found[0]


def inspect_resource(path: Path) -> dict:
    size = path.stat().st_size
    with path.open("rb") as stream:
        header = stream.read(3608)
    if len(header) != 3608:
        raise ValueError("Truncated Elma.res header")
    count = struct.unpack_from("<i", header)[0]
    if not 1 <= count <= 150 or struct.unpack_from("<i", header, 3604)[0] != 1347839:
        raise ValueError("Invalid Elma.res directory count/magic")
    candidates = []
    for edition, seed in [("registered", 9982), ("shareware", 9882)]:
        decoded = decode_directory(header[4:3604], seed)
        entries = []
        seen = set()
        for index in range(count):
            name_bytes, first, second = struct.unpack_from("<16sii", decoded, index * 24)
            if b"\0" not in name_bytes:
                break
            name = name_bytes.split(b"\0", 1)[0]
            offset, length = (second, first) if edition == "registered" else (first, second)
            if not name or any(value < 32 or value > 126 for value in name):
                break
            folded = name.lower()
            if folded in seen or offset < 3608 or length < 0 or offset + length > size:
                break
            seen.add(folded)
            entries.append({"name": name.decode("ascii"), "offset": offset, "length": length})
        else:
            candidates.append({"edition": edition, "resource_entries": count, "directory": entries})
    if len(candidates) != 1:
        raise ValueError("No unambiguous registered/shareware directory; data is damaged or unsupported")
    result = candidates[0]
    result["resource_size"] = size
    result["resource_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
    result["cmake_option"] = "-DELMA_SHAREWARE=OFF" if result["edition"] == "registered" else "-DELMA_SHAREWARE=ON"
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("data_dir", type=Path)
    parser.add_argument("--list", action="store_true", help="Include the resource directory in JSON output")
    args = parser.parse_args()
    try:
        root = args.data_dir.resolve(strict=True)
        result = inspect_resource(find_case(root, "Elma.res"))
        lgr = find_case(find_case(root, "Lgr"), "Default.lgr")
        with lgr.open("rb") as stream:
            signature = stream.read(5)
        if signature != b"LGR12":
            raise ValueError("Default.lgr is not the original LGR12 format")
        result["default_lgr_sha256"] = hashlib.sha256(lgr.read_bytes()).hexdigest()
        result["note"] = "Directory/header validation only, not a full asset audit or physics compatibility certificate"
        if not args.list:
            result.pop("directory")
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, struct.error) as error:
        parser.exit(1, f"Asset check failed: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
