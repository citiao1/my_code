"""Read-only image verification after programming; no halt or reset."""
import argparse
from intelhex import IntelHex
from pyocd.core.helpers import ConnectHelper

p = argparse.ArgumentParser()
p.add_argument("--pack", required=True)
p.add_argument("--probe", required=True)
p.add_argument("--image", required=True)
p.add_argument("--frequency", type=int, default=1_000_000)
p.add_argument("--critical-only", action="store_true",
                help="check device, vectors, marker, and run state without rereading the full image")
p.add_argument("--require-running", action="store_true")
a = p.parse_args()
image = IntelHex(a.image)
with ConnectHelper.session_with_chosen_probe(
    unique_id=a.probe, target_override="stm32f407vgtx",
    options={
        "pack": a.pack, "connect_mode": "attach", "frequency": a.frequency,
        "cmsis_dap.deferred_transfers": False,
        "cmsis_dap.limit_packets": True,
    },
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
    marker = next((addr for addr in range(0x08000000, 0x08100000 - 6)
                   if bytes(image[addr + i] for i in range(6)) == b"R18 F:"), None)
    if marker is None:
        raise RuntimeError("R18 marker is absent from the image")
    checks.append(marker & ~3)
    for address in checks:
        observed = target.read32(address)
        expected = sum(image[address + i] << (8 * i) for i in range(4))
        if observed != expected:
            raise RuntimeError(
                f"critical flash mismatch at {address:#010x}: "
                f"{observed:#010x} != {expected:#010x}"
            )
    verified_bytes = 0
    if not a.critical_only:
        for start, end in image.segments():
            for address in range(start, end, 256):
                count = min(256, end - address)
                expected = bytes(image[address + i] for i in range(count))
                # Prefer a single CMSIS-DAP block read. A few PUSO firmware
                # versions mishandle DAP_TRANSFER_BLOCK, so fall back to the
                # known-working word transfers when the block read fails.
                aligned_start = address & ~3
                aligned_end = (address + count + 3) & ~3
                try:
                    aligned = bytes(target.read_memory_block8(aligned_start,
                                                              aligned_end - aligned_start))
                    if len(aligned) != aligned_end - aligned_start:
                        raise RuntimeError("short DAP block read")
                    prefix = address - aligned_start
                    observed = aligned[prefix:prefix + count]
                except Exception:
                    words = [target.read32(word_address).to_bytes(4, "little")
                             for word_address in range(aligned_start, aligned_end, 4)]
                    prefix = address - aligned_start
                    observed = b"".join(words)[prefix:prefix + count]
                if observed != expected:
                    # Retry mismatching blocks with single-word transfers in
                    # case the probe returned stale/corrupt block data.
                    words = [target.read32(word_address).to_bytes(4, "little")
                             for word_address in range(aligned_start, aligned_end, 4)]
                    observed = b"".join(words)[prefix:prefix + count]
                    if observed != expected:
                        offset = next(i for i in range(count) if observed[i] != expected[i])
                        raise RuntimeError(f"flash image mismatch at {address + offset:#010x}")
                verified_bytes += count
                if verified_bytes % 8192 == 0:
                    print(f"Readback: {verified_bytes} bytes verified", flush=True)
    verification = (f"full image ({verified_bytes} bytes)" if not a.critical_only
                    else "critical addresses")
    print(f"VERIFIED {verification}, vector table and R18 marker at {marker:#010x}; "
          f"device {device_id:#010x}; flash {flash_kib} KiB")
    state = target.get_state().name
    print(f"Core state after programming: {state}")
    if a.require_running and state != "RUNNING":
        raise RuntimeError(f"target did not resume: {state}")
