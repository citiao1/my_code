"""Read-only flash comparison after programming; target remains running."""
import argparse
from intelhex import IntelHex
from pyocd.core.helpers import ConnectHelper

p = argparse.ArgumentParser()
p.add_argument("--pack", required=True)
p.add_argument("--probe", required=True)
p.add_argument("--image", required=True)
a = p.parse_args()
image = IntelHex(a.image)
with ConnectHelper.session_with_chosen_probe(
    unique_id=a.probe, target_override="stm32f407vgtx",
    options={"pack": a.pack, "connect_mode": "attach", "frequency": 1000000},
) as session:
    target = session.target
    device_id = target.read32(0xE0042000)
    flash_kib = target.read16(0x1FFF7A22)
    if (device_id & 0xFFF) != 0x413 or flash_kib != 1024:
        raise RuntimeError(f"unexpected device ID/flash: {device_id:#x}/{flash_kib} KiB")
    for start, end in image.segments():
        if not (0x08000000 <= start < end <= 0x08100000):
            raise RuntimeError("image contains data outside application flash")
    checks = [0x08000000, 0x08000004, 0x08000008]
    marker = next((addr for addr in range(0x08000000, 0x08100000 - 5)
                   if bytes(image[addr + i] for i in range(5)) == b"R7 F:"), None)
    if marker is None:
        raise RuntimeError("R7 marker is absent from the image")
    checks.append(marker & ~3)
    for address in checks:
        observed = target.read32(address)
        expected = sum(image[address + i] << (8 * i) for i in range(4))
        if observed != expected:
            raise RuntimeError(
                f"critical flash mismatch at {address:#010x}: "
                f"{observed:#010x} != {expected:#010x}"
            )
    print(f"VERIFIED vector table and R7 marker at {marker:#010x}; "
          f"device {device_id:#010x}; flash {flash_kib} KiB")
    print(f"Core state after programming: {target.get_state().name}")
