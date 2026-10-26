import tempfile
import unittest
from pathlib import Path
from intelhex import IntelHex
from flash_stm32_uart import FLASH_BASE, load_image, program


class FakeBoot:
    def __init__(self):
        self.data = bytearray([0xA5] * 32768)
        self.erased = None

    def read_memory_data(self, address, size):
        assert address == FLASH_BASE
        return self.data[:size]

    def erase_memory(self, sectors):
        assert sectors == [0, 1]  # Explicit sector list, never global erase.
        self.erased = list(sectors)
        self.data[:] = bytes([255]) * len(self.data)

    def write_memory_data(self, address, data):
        assert address == FLASH_BASE
        self.data[:] = data

    def verify_data(self, actual, expected):
        assert actual == expected


class FlashTests(unittest.TestCase):
    def test_sparse_hex_preserves_unwritten_bytes_and_sector_tails(self):
        boot = FakeBoot()
        image = IntelHex({FLASH_BASE: 0x55, FLASH_BASE + 17000: 0x11})
        record = {}
        with tempfile.TemporaryDirectory() as folder:
            program(boot, image, image.addresses(), Path(folder), record)
            self.assertEqual((Path(folder) / "before.bin").read_bytes(), bytes([0xA5])*32768)
        self.assertEqual(boot.erased, [0, 1])
        self.assertEqual(boot.data[0], 0x55)
        self.assertEqual(boot.data[17000], 0x11)
        self.assertEqual(boot.data[1:17000], bytes([0xA5])*16999)
        self.assertEqual(boot.data[17001:], bytes([0xA5])*(32768-17001))
        self.assertTrue(record["verified"])

    def test_reject_invalid_vectors_before_connecting(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bad.hex"
            IntelHex({FLASH_BASE: 0}).write_hex_file(str(path))
            with self.assertRaisesRegex(ValueError, "stack pointer"):
                load_image(path)


if __name__ == "__main__":
    unittest.main()
