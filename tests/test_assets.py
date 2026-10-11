"""Asset inspector tests use synthetic directory records, not copyrighted assets."""
import ctypes
import importlib.util
import math
from pathlib import Path
import struct
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location("asset_checker", Path(__file__).resolve().parents[1] / "tools" / "check_assets.py")
CHECKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECKER)


def fixture(edition="registered", offset=3608, duplicate=False):
    count = 2 if duplicate else 1
    raw = bytearray(3600)
    for index in range(count):
        values = (8, offset) if edition == "registered" else (offset, 8)
        struct.pack_into("<16sii", raw, 24 * index, b"TEST.DAT", *values)
    a = 23
    b = 9982 if edition == "registered" else 9882
    for index in range(len(raw)):
        raw[index] ^= a & 255
        a = a - math.trunc(a / 3391) * 3391
        b = ctypes.c_int16(b + a * 3391).value
        a = ctypes.c_int16(31 * b + 3391).value
    return struct.pack("<i", count) + raw + struct.pack("<i", 1347839) + b"ABCDEFGH"


class AssetTests(unittest.TestCase):
    def inspect(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "Elma.res"
            path.write_bytes(data)
            return CHECKER.inspect_resource(path)

    def test_registered(self):
        result = self.inspect(fixture())
        self.assertEqual(result["edition"], "registered")
        self.assertEqual(result["directory"], [{"name": "TEST.DAT", "offset": 3608, "length": 8}])

    def test_shareware(self):
        self.assertEqual(self.inspect(fixture("shareware"))["edition"], "shareware")

    def test_truncated(self):
        with self.assertRaises(ValueError):
            self.inspect(fixture()[:3500])

    def test_bad_extent(self):
        with self.assertRaises(ValueError):
            self.inspect(fixture(offset=9000))

    def test_duplicate(self):
        with self.assertRaises(ValueError):
            self.inspect(fixture(duplicate=True))

    def test_magic(self):
        raw = bytearray(fixture())
        raw[3604] ^= 1
        with self.assertRaises(ValueError):
            self.inspect(raw)

    def test_case_lookup(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "DeFaUlT.Lgr").write_bytes(b"LGR12")
            self.assertEqual(CHECKER.find_case(root, "default.lgr").read_bytes(), b"LGR12")


if __name__ == "__main__":
    unittest.main()
