#!/usr/bin/env python3
"""Passively capture BM50 wheel, yaw-rate, or heading step-response data."""

from __future__ import annotations

import argparse
import asyncio
import csv
import json
import math
import sys
import time
from pathlib import Path
from typing import Any

import websockets


BRIDGE_GREETING = "BRIDGE,BM50_ACK_R3"
FIRMWARE_VERSIONS = (
    "VER,VGT6_BM50_SPEED_FIXED_R5",
    "VER,VGT6_BM50_SPEED_TUNABLE_R6",
    "VER,VGT6_BM50_ATTITUDE_R10",
)
FIRMWARE_VERSION = FIRMWARE_VERSIONS[-1]
ATTITUDE_CAPS = "CAPS,ATTITUDE_R10,20,10,200,60,60,80,SPEED_TUNABLE,HLOG"
BRIDGE_URL = "ws://127.0.0.1:8766"
REQUEST_TIMEOUT_SECONDS = 5.0
MOTOR_INDEX = {"A": 0, "B": 1, "C": 2, "D": 3}
LOOP_CHOICES = ("wheel_speed", "angular_velocity", "direction")
WHEEL_DIAMETER_M = 0.127
ENCODER_COUNTS_PER_REV = 380
METERS_PER_ENCODER_COUNT = math.pi * WHEEL_DIAMETER_M / ENCODER_COUNTS_PER_REV


class BridgeClient:
    def __init__(
        self,
        websocket: Any,
        *,
        expected_firmware: str | tuple[str, ...] = FIRMWARE_VERSIONS,
        require_fixed_gains: bool = False,
    ) -> None:
        self.websocket = websocket
        self.request_id = 0
        self.telemetry_was_enabled: bool | None = None
        self.expected_firmware = expected_firmware
        self.require_fixed_gains = require_fixed_gains
        self.firmware_version = ""
        self.gains: list[float] = []
        self.telemetry_started = False

    async def command(self, command: str) -> str:
        self.request_id += 1
        request_id = self.request_id
        await self.websocket.send(json.dumps({
            "type": "command", "id": request_id, "command": command,
        }))
        deadline = asyncio.get_running_loop().time() + REQUEST_TIMEOUT_SECONDS
        while True:
            remaining = deadline - asyncio.get_running_loop().time()
            if remaining <= 0:
                raise TimeoutError(f"bridge response timeout: {command}")
            message = await asyncio.wait_for(self.websocket.recv(), remaining)
            if not isinstance(message, str) or not message.startswith("{"):
                continue
            result = json.loads(message)
            if result.get("id") != request_id:
                continue
            if not result.get("ok"):
                raise RuntimeError(result.get("error", f"command failed: {command}"))
            return str(result.get("reply", ""))

    async def prepare(self) -> None:
        version = await self.command("VER")
        self.firmware_version = version
        expected_versions = (
            (self.expected_firmware,)
            if isinstance(self.expected_firmware, str)
            else tuple(self.expected_firmware)
        )
        if expected_versions and version not in expected_versions:
            raise RuntimeError(f"unsupported firmware: {version}")

        stack = (await self.command("STACK")).split(",")
        if len(stack) != 5 or stack[0] != "STACK" or stack[4] != "1":
            raise RuntimeError("firmware stack guard is unhealthy")

        link = (await self.command("LINK")).split(",")
        if len(link) != 7 or link[0] != "LINK":
            raise RuntimeError("invalid LINK response")
        self.telemetry_was_enabled = link[6] == "1"

        gains = (await self.command("PIDGET")).split(",")
        if len(gains) != 13 or gains[0] != "PID":
            raise RuntimeError("invalid PIDGET response")
        values = [float(value) for value in gains[1:]]
        self.gains = values
        expected_gains = (
            [x for ki in (0.0991, 0.0998, 0.0998, 0.1008) for x in (1.2, ki, 0.0)]
            if version == FIRMWARE_VERSIONS[-1]
            else [1.2, 0.2, 0.0] * 4
        )
        if self.require_fixed_gains and version != "VER,VGT6_BM50_ATTITUDE_R10" and not all(
            math.isclose(value, expected, rel_tol=0.0, abs_tol=0.0001)
            for value, expected in zip(values, expected_gains)
        ):
            raise RuntimeError("unexpected PID gains; refusing capture")

    async def restore_telemetry(self) -> None:
        if self.telemetry_started:
            try:
                await self.command("TEL,0")
            except Exception as error:
                print(f"Warning: could not restore telemetry state: {error}", file=sys.stderr)


def telemetry_row(
    message: str,
    motor_index: int | None,
    first_tick: int | None,
    loop_name: str = "wheel_speed",
):
    fields = message.strip().split(",")
    expected_tag = "TEL" if loop_name == "wheel_speed" else "ATEL"
    expected_count = 27 if loop_name == "wheel_speed" else 14
    if len(fields) != expected_count or fields[0] != expected_tag:
        return None
    try:
        values = [float(x) for x in fields[1:]]
    except ValueError:
        return None
    if not all(math.isfinite(x) for x in values):
        return None
    if values[2 if loop_name == "wheel_speed" else 11] != 1:
        return None
    if loop_name != "wheel_speed" and (
        values[2] != 1 or values[1] != (1 if loop_name == "angular_velocity" else 2)
    ):
        return None
    tick = values[0]
    if first_tick is None:
        first_tick = tick
    time_s = ((int(tick) - int(first_tick)) & 0xFFFFFFFF) / 1000.0

    if loop_name == "wheel_speed":
        if motor_index is None:
            return None
        if motor_index not in range(4):
            return None
        return (
            time_s,
            values[3 + motor_index],
            values[11 + motor_index] / 10.0,
            values[7 + motor_index],
        ), first_tick

    if loop_name == "angular_velocity":
        # target yaw rate, filtered gyro rate, and yaw-loop RPM output.
        return (
            time_s,
            values[3] / 1000.0,
            values[4] / 1000.0,
            values[5] / 1000.0,
        ), first_tick
    if loop_name == "direction":
        # Wrapped heading target/feedback and the outer-loop yaw-rate output.
        return (
            time_s,
            values[6] / 1000.0,
            values[7] / 1000.0,
            values[9] / 1000.0,
        ), first_tick
    return None


def encoder_row(message: str):
    """Return (tick, ready, cpr, totals) for an ENC diagnostic packet."""
    fields = message.strip().split(",")
    if len(fields) != 16 or fields[0] != "ENC":
        return None
    try:
        values = [int(x, 0) for x in fields[1:]]
    except ValueError:
        return None
    tick, ready, cpr = values[:3]
    totals = tuple(values[3:7])
    if cpr <= 0 or not all(math.isfinite(float(x)) for x in values):
        return None
    return tick, ready, cpr, totals


def hlog_row(message: str):
    """Parse HLOG; legacy records have unknown mode, not inferred rate data."""
    fields = message.strip().split(",")
    if len(fields) not in (14, 15) or fields[:2] != ["HLOG", "DATA"]:
        return None
    try:
        values = [int(value, 0) for value in fields[2:]]
    except ValueError:
        return None
    if values[0] < 0 or values[1] < 0 or values[9] not in (0, 1):
        return None
    if len(values) == 13 and values[12] not in (1, 2):
        return None
    return {
        "sequence": values[0],
        "tick": values[1],
        "target_heading_deg": values[2] / 1000.0,
        "actual_heading_deg": values[3] / 1000.0,
        "heading_error_deg": values[4] / 1000.0,
        "target_rate_dps": values[5] / 1000.0,
        "actual_rate_dps": values[6] / 1000.0,
        "heading_output_dps": values[7] / 1000.0,
        "rate_output_rpm": values[8] / 1000.0,
        "saturated": bool(values[9]),
        "forward_rpm": values[10],
        "left_rpm": values[11],
        "mode": values[12] if len(values) == 13 else None,
    }


def hlog_samples(records, loop="direction", *, max_interval_ms=60):
    """Strict high-rate log -> controller samples; no live-telemetry fallback."""
    expected_mode = {"angular_velocity": 1, "direction": 2}.get(loop)
    if expected_mode is None or not records:
        raise ValueError("nonempty supported high-rate log required")
    elapsed = 0
    rows = []
    previous_tick = None
    for sequence, record in enumerate(records):
        if record["sequence"] != sequence or record.get("mode") != expected_mode:
            raise ValueError("HLOG sequence incomplete or wrong/missing loop mode")
        tick = record["tick"]
        if not isinstance(tick, int) or not 0 <= tick <= 0xffffffff:
            raise ValueError("HLOG MCU timestamp out of range")
        if previous_tick is not None:
            dt = (tick - previous_tick) & 0xffffffff
            if not 20 <= dt <= max_interval_ms:
                raise ValueError("HLOG timestamp duplicate, unordered or gap")
            elapsed += dt
        previous_tick = tick
        keys = ("target_rate_dps", "actual_rate_dps", "rate_output_rpm") if expected_mode == 1 else (
            "target_heading_deg", "actual_heading_deg", "heading_output_dps")
        values = tuple(record[key] for key in keys)
        if not all(math.isfinite(value) for value in values):
            raise ValueError("HLOG nonfinite controller data")
        rows.append((elapsed / 1000.0, *values))
    return rows


def distance_from_encoder(
    totals: tuple[int, int, int, int],
    origin: tuple[int, int, int, int],
    *,
    meters_per_count: float = METERS_PER_ENCODER_COUNT,
) -> float:
    """Estimate forward distance from the signed mean of four wheel totals."""
    if len(totals) != 4 or len(origin) != 4 or meters_per_count <= 0:
        raise ValueError("invalid encoder distance inputs")
    displacement = sum(float(now - start)
                      for now, start in zip(totals, origin)) / 4.0
    return abs(displacement) * meters_per_count


def speed_drive_row(message: str, first_tick: int | None = None):
    """Convert a TEL packet to vehicle forward RPM/PWM averages."""
    fields = message.strip().split(",")
    if len(fields) != 27 or fields[0] != "TEL":
        return None
    try:
        values = [int(x, 0) for x in fields[1:]]
    except ValueError:
        return None
    if values[2] != 1:
        return None
    tick = values[0]
    if first_tick is None:
        first_tick = tick
    time_s = ((tick - first_tick) & 0xFFFFFFFF) / 1000.0
    actual = sum(values[11:15]) / 40.0
    output = sum(values[7:11]) / 4.0
    target = sum(values[3:7]) / 4.0
    return (time_s, target, actual, output), first_tick


async def capture(
    motor: str | None,
    output: Path,
    url: str = BRIDGE_URL,
    *,
    expected_firmware: str | tuple[str, ...] = FIRMWARE_VERSIONS,
    require_fixed_gains: bool = False,
    loop_name: str = "bm50-wheel-speed",
    controller_form: str = "incremental",
    control_period_s: float = 0.01,
    loop: str = "wheel_speed",
) -> None:
    if loop not in LOOP_CHOICES:
        raise ValueError(f"unsupported loop: {loop}")
    if loop == "wheel_speed" and motor not in MOTOR_INDEX:
        raise ValueError("wheel_speed capture requires --motor A/B/C/D")
    if loop != "wheel_speed":
        motor = None
        controller_form = "parallel"
        control_period_s = 0.02
    output.parent.mkdir(parents=True, exist_ok=True)
    async with websockets.connect(url, open_timeout=REQUEST_TIMEOUT_SECONDS) as websocket:
        greeting = await asyncio.wait_for(websocket.recv(), REQUEST_TIMEOUT_SECONDS)
        if isinstance(greeting, str):
            greeting = greeting.strip()
        if greeting != BRIDGE_GREETING:
            raise RuntimeError(f"unexpected bridge greeting: {greeting!r}")
        client = BridgeClient(
            websocket,
            expected_firmware=expected_firmware,
            require_fixed_gains=require_fixed_gains and loop == "wheel_speed",
        )
        await client.prepare()
        # Do not silently replace another client's already-selected stream.
        if client.telemetry_was_enabled:
            raise RuntimeError("disable existing telemetry before passive capture")
        if loop != "wheel_speed":
            caps = await client.command("CAPS")
            if caps != ATTITUDE_CAPS:
                raise RuntimeError("unsupported attitude capability")
        await client.command("TEL,1" if loop == "wheel_speed" else "ATEL,1")
        client.telemetry_started = True
        metadata_path = output.with_suffix(output.suffix + ".meta.json")
        metadata_path.write_text(json.dumps({
            "loop_name": loop_name,
            "motor": motor,
            "loop": loop,
            "controller_form": controller_form,
            "control_period_s": control_period_s,
            "telemetry_period_s": 0.3 if loop == "wheel_speed" else 0.2,
            "setpoint_unit": {
                "wheel_speed": "rpm",
                "angular_velocity": "deg/s",
                "direction": "deg",
            }[loop],
            "process_unit": {
                "wheel_speed": "rpm",
                "angular_velocity": "deg/s",
                "direction": "deg",
            }[loop],
            "output_unit": {
                "wheel_speed": "percent",
                "angular_velocity": "rpm",
                "direction": "deg/s",
            }[loop],
            "firmware": client.firmware_version,
            "gains": client.gains,
            "source": "VGT6 TEL/ATEL over local WebSocket bridge",
        }, indent=2), encoding="utf-8")
        print(f"Recording {loop} {motor or ''} to {output}. Press Ctrl+C to stop.")
        first_tick = None
        try:
            with output.open("w", newline="", encoding="utf-8") as stream:
                writer = csv.writer(stream)
                headers = {
                    "wheel_speed": ("time_s", "setpoint_rpm", "process_rpm", "output_percent"),
                    "angular_velocity": ("time_s", "setpoint_dps", "process_dps", "output_rpm"),
                    "direction": ("time_s", "setpoint_deg", "process_deg", "output_dps"),
                }[loop]
                writer.writerow(headers)
                async for message in websocket:
                    if not isinstance(message, str):
                        continue
                    parsed = telemetry_row(
                        message,
                        MOTOR_INDEX[motor] if motor else None,
                        first_tick,
                        loop,
                    )
                    if parsed is None:
                        continue
                    row, first_tick = parsed
                    writer.writerow(row)
                    stream.flush()
        finally:
            await client.restore_telemetry()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--motor", choices=MOTOR_INDEX)
    parser.add_argument(
        "--loop",
        choices=LOOP_CHOICES,
        default="wheel_speed",
        help="wheel_speed, angular_velocity, or direction",
    )
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--url", default=BRIDGE_URL)
    parser.add_argument("--expected-firmware", default=FIRMWARE_VERSION)
    parser.add_argument("--allow-gain-mismatch", action="store_true")
    parser.add_argument("--loop-name", default="bm50-wheel-speed")
    parser.add_argument(
        "--controller-form",
        choices=("parallel", "incremental", "position-discrete"),
        default="incremental",
    )
    parser.add_argument("--control-period", type=float, default=0.01)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        asyncio.run(capture(
            args.motor,
            args.output,
            args.url,
            expected_firmware=args.expected_firmware,
            require_fixed_gains=not args.allow_gain_mismatch,
            loop_name=args.loop_name,
            controller_form=args.controller_form,
            control_period_s=args.control_period,
            loop=args.loop,
        ))
    except KeyboardInterrupt:
        print("\nCapture stopped.")
    except Exception as error:
        print(f"Adapter error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
