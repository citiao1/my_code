"""Headless STM32F407 UART programming with the board's FlyMCU reset wiring.

Uses stm32loader's AN3155 transport, not FlyMCU UI automation. Never invokes
unprotect, option-byte writes or mass erase. Preserve bytes outside the HEX
image even within touched flash sectors. The default operation is read-only.
"""
import argparse
import hashlib
import json
from datetime import datetime
from pathlib import Path

from intelhex import IntelHex
from stm32loader.bootloader import Stm32Bootloader
from stm32loader.uart import SerialConnection

FLASH_BASE = 0x08000000
SECTOR_SIZES = [16 * 1024] * 4 + [64 * 1024] + [128 * 1024] * 7


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def load_image(path):
    image = IntelHex(str(path))
    addresses = image.addresses()
    if not addresses or min(addresses) != FLASH_BASE:
        raise ValueError("HEX must start at 0x08000000")
    if max(addresses) >= FLASH_BASE + 512 * 1024:
        raise ValueError("HEX exceeds the STM32F407VET6 512 KiB flash")
    vectors = bytes(image.tobinarray(start=FLASH_BASE, size=8))
    stack = int.from_bytes(vectors[:4], "little")
    reset = int.from_bytes(vectors[4:], "little")
    if not 0x20000000 < stack <= 0x20020000:
        raise ValueError("Invalid initial stack pointer")
    if not reset & 1 or not FLASH_BASE <= (reset & ~1) <= max(addresses):
        raise ValueError("Invalid reset vector")
    return image, addresses


def program(boot, image, addresses, record_dir, record):
    last = max(addresses)
    end = FLASH_BASE
    sectors = []
    for index, size in enumerate(SECTOR_SIZES):
        sectors.append(index)
        end += size
        if last < end:
            break
    count = end - FLASH_BASE
    print(f"Backing up {count} bytes; affected sectors: {sectors}", flush=True)
    before = bytes(boot.read_memory_data(FLASH_BASE, count))
    if len(before) != count:
        raise RuntimeError("Incomplete backup; refusing erase")
    backup = record_dir / "before.bin"
    backup.write_bytes(before)
    expected = bytearray(before)
    for address in addresses:
        expected[address - FLASH_BASE] = image[address]
    record.update(sectors=sectors, preserved_bytes=count-len(addresses),
                  backup=str(backup), before_sha256=sha256(before))
    (record_dir / "result.json").write_text(json.dumps(record, indent=2), encoding="utf-8")
    print("Erasing only image sectors (no mass erase)", flush=True)
    boot.erase_memory(sectors)
    print(f"Writing {count} bytes including preserved sector tails", flush=True)
    boot.write_memory_data(FLASH_BASE, expected)
    print("Reading back every written byte", flush=True)
    actual = bytes(boot.read_memory_data(FLASH_BASE, count))
    boot.verify_data(actual, expected)
    record.update(verified=True, verified_bytes=count, readback_sha256=sha256(actual))
    print(f"VERIFY OK: {count} bytes, SHA256 {sha256(actual)}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="COM7")
    parser.add_argument("--baud", type=int, default=76800)
    parser.add_argument("--image", type=Path)
    parser.add_argument("--flash", action="store_true", help="write and verify; otherwise probe only")
    args = parser.parse_args()
    if args.flash and not args.image:
        parser.error("--flash requires --image")
    image, addresses = load_image(args.image) if args.flash else (None, None)
    record = dict(port=args.port, baud=args.baud, verified=False, reset_to_flash=False)
    if args.flash:
        record.update(image=str(args.image.resolve()), image_sha256=sha256(args.image.read_bytes()))
    record_dir = Path(__file__).resolve().parents[1] / "flash-records" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    record_dir.mkdir(parents=True)
    connection = SerialConnection(args.port, args.baud, "E")
    # Match FlyMCU's observed DTR-low reset / RTS-high BootLoader sequence.
    connection.reset_active_high = True
    connection.boot0_active_low = True
    boot = Stm32Bootloader(connection, verbosity=0, device_family="F4")
    writing = False
    reset_allowed = True
    try:
        connection.connect()
        print(f"Connecting to {args.port} @ {args.baud}, 8E1", flush=True)
        boot.reset_from_system_memory()
        version = boot.get()
        chip = boot.get_id()
        if chip != 0x413:
            raise RuntimeError(f"Unexpected chip ID 0x{chip:04X}; expected 0x0413")
        flash_kib, _uid = boot.get_flash_size_and_uid()
        if flash_kib != 512:
            raise RuntimeError(f"Expected 512 KiB, found {flash_kib} KiB")
        record.update(chip_id=f"0x{chip:04X}", bootloader=f"0x{version:02X}", flash_kib=flash_kib)
        print(f"Chip 0x{chip:04X}, bootloader 0x{version:02X}, flash {flash_kib} KiB", flush=True)
        if args.flash:
            writing = True
            reset_allowed = False
            program(boot, image, addresses, record_dir, record)
            reset_allowed = True
        boot.reset_from_flash()
        record["reset_to_flash"] = True
        print("Reset to flash issued; physical I/O not verified", flush=True)
    except Exception as error:
        record["error"] = str(error)
        # After an interrupted erase/write, leave ROM bootloader available for retry.
        if connection.serial_connection and reset_allowed:
            boot.reset_from_flash()
        raise
    finally:
        record["write_requested"] = writing
        connection.disconnect()
        (record_dir / "result.json").write_text(json.dumps(record, indent=2), encoding="utf-8")
        print(f"Record: {record_dir / 'result.json'}", flush=True)


if __name__ == "__main__":
    main()
