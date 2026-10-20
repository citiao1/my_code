"""Guarded live PI tuning for the four BM50 wheel speed loops.

This is a bench-session tool. It owns only the WebSocket client, never the
BLE/serial link, and always sends an explicit STOP during cleanup.
"""

from __future__ import annotations

import argparse
import asyncio
import csv
import json
import time
from dataclasses import asdict
from pathlib import Path

import websockets

from pid_tuning import Sample, identify_fopdt, tune_fopdt


WHEELS = ("A", "B", "C", "D")
TARGET_LIMIT = 160
OUTPUT_LIMIT = 60.0
CONTROL_PERIOD_S = 0.01
DEFAULT_GAINS = (1.2, 0.2, 0.0)
MAX_SAFE_KP = 4.0
MAX_SAFE_KI = 0.8
MAX_SAFE_KD = 0.0
MIN_STEP_SAMPLES = 8
MAX_GAIN_CHANGE_FRACTION = 0.20


class LiveSessionError(RuntimeError):
    pass


class BridgeSession:
    def __init__(self, url: str):
        self.url = url
        self.ws = None
        self.next_id = 1
        self.baseline_link_errors = None
        self.baseline_uart_errors = None
        self.last_link = None
        self.initial_gains: tuple[tuple[float, float, float], ...] | None = None

    async def __aenter__(self):
        self.ws = await websockets.connect(self.url, open_timeout=5, close_timeout=2)
        greeting = await asyncio.wait_for(self.ws.recv(), timeout=5)
        if isinstance(greeting, bytes):
            greeting = greeting.decode("utf-8", "replace")
        if str(greeting).strip() != "BRIDGE,BM50_ACK_R3":
            raise LiveSessionError(f"unexpected bridge greeting: {greeting!r}")
        return self

    async def __aexit__(self, exc_type, exc, tb):
        try:
            await self.command("STOP", timeout=3)
        except Exception as stop_error:
            print(f"STOP cleanup failed: {stop_error}")
        if self.ws is not None:
            await self.ws.close()

    async def command(self, command: str, timeout: float = 4.0) -> str:
        if self.ws is None:
            raise LiveSessionError("WebSocket is not connected")
        request_id = self.next_id
        self.next_id += 1
        await self.ws.send(
            json.dumps({"type": "command", "id": request_id, "command": command})
        )
        deadline = asyncio.get_running_loop().time() + timeout
        while True:
            remaining = deadline - asyncio.get_running_loop().time()
            if remaining <= 0:
                raise LiveSessionError(f"command timeout: {command}")
            message = await asyncio.wait_for(self.ws.recv(), timeout=remaining)
            if isinstance(message, bytes):
                message = message.decode("utf-8", "replace")
            try:
                payload = json.loads(message)
            except (TypeError, ValueError):
                continue
            if payload.get("type") == "result" and payload.get("id") == request_id:
                if not payload.get("ok"):
                    raise LiveSessionError(
                        f"command rejected: {command}: {payload.get('reply')}"
                    )
                return str(payload.get("reply", ""))

    async def preflight(self) -> dict[str, str]:
        replies = {}
        for command in ("VER", "CAPS", "STACK", "LINK", "SPEEDLIMIT", "PIDGET"):
            replies[command] = await self.command(command)
        if replies["VER"] not in (
            "VER,VGT6_BM50_SPEED_TUNABLE_R6",
            "VER,VGT6_BM50_ATTITUDE_R10",
        ):
            raise LiveSessionError(f"unsupported firmware: {replies['VER']}")
        caps = replies["CAPS"].split(",")
        if len(caps) < 2 or caps[0] != "CAPS" or caps[-1] != "SPEED_TUNABLE":
            raise LiveSessionError(f"speed PID write capability missing: {replies['CAPS']}")
        pid_fields = replies["PIDGET"].split(",")
        if len(pid_fields) != 13 or pid_fields[0] != "PID":
            raise LiveSessionError(f"malformed PIDGET: {replies['PIDGET']}")
        values = tuple(float(value) for value in pid_fields[1:])
        if any(value < 0.0 or value > 100.0 for value in values):
            raise LiveSessionError("PIDGET gain outside 0..100")
        self.initial_gains = tuple(
            tuple(values[offset:offset + 3]) for offset in range(0, 12, 3)
        )
        stack = replies["STACK"].split(",")
        if len(stack) != 5 or stack[0] != "STACK" or stack[4] != "1":
            raise LiveSessionError(f"stack guard failed: {replies['STACK']}")
        if int(stack[1]) < 4096 or int(stack[3]) < 32:
            raise LiveSessionError(f"insufficient stack margin: {replies['STACK']}")
        limit = replies["SPEEDLIMIT"].split(",")
        if len(limit) != 2 or int(limit[1]) != TARGET_LIMIT:
            await self.command(f"SPDLIMIT,{TARGET_LIMIT}")
            checked = await self.command("SPEEDLIMIT")
            if checked != f"SPEEDLIMIT,{TARGET_LIMIT}":
                raise LiveSessionError(f"target limit mismatch: {checked}")
        link = replies["LINK"].split(",")
        if len(link) < 7 or link[0] != "LINK":
            raise LiveSessionError(f"malformed LINK: {replies['LINK']}")
        self.baseline_link_errors = int(link[3])
        self.baseline_uart_errors = int(link[5])
        self.last_link = replies["LINK"]
        return replies

    async def get_pid_checked(self, wheel: str) -> tuple[float, float, float]:
        readback = await self.command("PIDGET")
        values = readback.split(",")
        if len(values) != 13 or values[0] != "PID":
            raise LiveSessionError(f"malformed PIDGET: {readback}")
        index = WHEELS.index(wheel) * 3 + 1
        return tuple(float(values[index + offset]) for offset in range(3))

    async def check_health(self) -> None:
        stack = (await self.command("STACK")).split(",")
        link = (await self.command("LINK")).split(",")
        if len(stack) != 5 or stack[0] != "STACK" or stack[4] != "1":
            raise LiveSessionError(f"stack guard failed: {','.join(stack)}")
        if int(stack[3]) < 32:
            raise LiveSessionError(f"stack margin too small: {','.join(stack)}")
        if len(link) < 7 or link[0] != "LINK":
            raise LiveSessionError(f"malformed LINK: {','.join(link)}")
        if int(link[3]) != self.baseline_link_errors:
            raise LiveSessionError(f"RX overflow changed: {','.join(link)}")
        if int(link[5]) != self.baseline_uart_errors:
            raise LiveSessionError(f"UART errors changed: {','.join(link)}")
        self.last_link = ",".join(link)

    async def set_pid_checked(self, wheel: str, gains: tuple[float, float, float]):
        kp, ki, kd = gains
        current = await self.get_pid_checked(wheel)
        for name, before, requested in zip(("kp", "ki", "kd"), current, gains):
            if before == 0.0:
                if abs(requested) > 1e-6:
                    raise LiveSessionError(
                        f"{wheel} {name} cannot change from zero in one write: "
                        f"current={before} requested={requested}"
                    )
                continue
            if abs(requested - before) > abs(before) * MAX_GAIN_CHANGE_FRACTION + 1e-6:
                raise LiveSessionError(
                    f"{wheel} {name} change exceeds "
                    f"{MAX_GAIN_CHANGE_FRACTION:.0%}: current={before} requested={requested}"
                )
        command = f"PID,{wheel},{kp:.6f},{ki:.6f},{kd:.6f}"
        reply = await self.command(command)
        if reply != "ACK,PID":
            raise LiveSessionError(f"PID write rejected: {reply}")
        actual = await self.get_pid_checked(wheel)
        if any(abs(a - b) > 1e-4 for a, b in zip(actual, gains)):
            raise LiveSessionError(f"PID readback mismatch: requested={gains} got={actual}")
        return actual

    async def start_speed_loop(self) -> None:
        await self.command("SPDLIMIT,160")
        await self.command("ARM")
        await self.command("SPEED,1")
        await self.command("TEL,1")
        checked = await self.command("SPEEDLIMIT")
        if checked != "SPEEDLIMIT,160":
            raise LiveSessionError(f"target ceiling changed: {checked}")

    async def set_target(self, wheel: str, rpm: int) -> None:
        reply = await self.command(f"SPD,{wheel},{rpm}")
        if reply != "ACK,SPD":
            raise LiveSessionError(f"target rejected: {reply}")

    async def collect(
        self,
        wheel: str,
        target: int,
        duration_s: float,
        samples: list[dict],
        stop_at: float,
    ) -> None:
        wheel_index = WHEELS.index(wheel)
        while time.monotonic() < stop_at:
            remaining = max(0.05, stop_at - time.monotonic())
            try:
                message = await asyncio.wait_for(self.ws.recv(), timeout=min(0.5, remaining))
            except asyncio.TimeoutError:
                await self.check_health()
                continue
            if isinstance(message, bytes):
                message = message.decode("utf-8", "replace")
            if not isinstance(message, str) or not message.startswith("TEL,"):
                continue
            fields = message.split(",")
            if len(fields) < 16:
                raise LiveSessionError(f"short TEL: {message}")
            if fields[0] != "TEL" or fields[3] != "1":
                continue
            targets = [int(fields[4 + index]) for index in range(4)]
            outputs = [float(fields[8 + index]) for index in range(4)]
            actuals = [float(fields[12 + index]) / 10.0 for index in range(4)]
            if any(abs(value) > OUTPUT_LIMIT + 1e-6 for value in outputs):
                raise LiveSessionError(f"output clamp exceeded: {message}")
            if any(targets[index] != (target if index == wheel_index else 0)
                   for index in range(4)):
                raise LiveSessionError(f"unexpected target coupling: {message}")
            samples.append(
                {
                    "tick_ms": int(fields[1]),
                    "target_rpm": targets[wheel_index],
                    "actual_rpm": actuals[wheel_index],
                    "output_percent": outputs[wheel_index],
                }
            )

    async def run_stage(
        self,
        wheel: str,
        target: int,
        duration_s: float,
        samples: list[dict],
    ) -> None:
        await self.set_target(wheel, target)
        await self.collect(
            wheel,
            target,
            duration_s,
            samples,
            time.monotonic() + duration_s,
        )
        await self.check_health()


def save_csv(path: Path, samples: list[dict]) -> None:
    if not samples:
        raise LiveSessionError("no TEL samples collected")
    first_tick = samples[0]["tick_ms"]
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["time_s", "setpoint_rpm", "process_rpm", "output_percent"])
        for sample in samples:
            writer.writerow(
                [
                    round((sample["tick_ms"] - first_tick) / 1000.0, 3),
                    sample["target_rpm"],
                    sample["actual_rpm"],
                    sample["output_percent"],
                ]
            )


def extract_step(samples: list[dict], target: int) -> list[Sample]:
    target_index = next(
        (index for index, sample in enumerate(samples) if sample["target_rpm"] == target),
        None,
    )
    if target_index is None:
        raise LiveSessionError(f"target step {target} not found")
    end = next(
        (
            index
            for index in range(target_index + 1, len(samples))
            if samples[index]["target_rpm"] != target
        ),
        len(samples),
    )
    start = max(0, target_index - 12)
    selected = samples[start:end]
    first_tick = selected[0]["tick_ms"]
    return [
        Sample(
            time_s=(sample["tick_ms"] - first_tick) / 1000.0,
            setpoint=float(sample["target_rpm"]),
            process=float(sample["actual_rpm"]),
            output=float(sample["output_percent"]),
        )
        for sample in selected
    ]


def conservative_candidate(samples: list[dict]) -> tuple[tuple[float, float, float], dict]:
    step_samples = extract_step(samples, 80)
    if len(step_samples) < MIN_STEP_SAMPLES:
        raise LiveSessionError(f"not enough step samples: {len(step_samples)}")
    model = identify_fopdt(step_samples)
    candidate = tune_fopdt(
        model,
        mode="PI",
        form="incremental",
        sample_time_s=CONTROL_PERIOD_S,
        closed_loop_time_s=0.10,
        output_limit=OUTPUT_LIMIT,
    )
    if model.saturated:
        raise LiveSessionError("80 RPM identification step is saturated")
    kp = min(max(candidate.kp, 0.0), MAX_SAFE_KP)
    ki = min(max(candidate.ki, 0.0), MAX_SAFE_KI)
    kd = min(max(candidate.kd, 0.0), MAX_SAFE_KD)
    if kp < 0.15:
        kp = DEFAULT_GAINS[0]
    if ki < 0.05:
        ki = DEFAULT_GAINS[1]
    return (kp, ki, kd), {
        "model": asdict(model),
        "candidate_raw": asdict(candidate),
        "candidate_applied": {"kp": kp, "ki": ki, "kd": kd},
    }


def evaluate(samples: list[dict], wheel: str) -> dict:
    by_target: dict[int, list[dict]] = {}
    for sample in samples:
        target = int(sample["target_rpm"])
        if target in (40, 80, 120, 160):
            by_target.setdefault(target, []).append(sample)
    result = {}
    for target, values in by_target.items():
        tail = values[-min(8, len(values)):]
        actual = [float(item["actual_rpm"]) for item in tail]
        output = [abs(float(item["output_percent"])) for item in tail]
        if not actual:
            continue
        result[str(target)] = {
            "samples": len(values),
            "tail_mean_rpm": round(sum(actual) / len(actual), 2),
            "tail_max_error_rpm": round(max(abs(target - value) for value in actual), 2),
            "tail_output_mean_percent": round(sum(output) / len(output), 2),
            "saturated": max(output) >= OUTPUT_LIMIT - 1e-6,
        }
    return {"wheel": wheel, "validation": result}


async def tune_wheel(
    session: BridgeSession,
    wheel: str,
    output_dir: Path,
) -> dict:
    if session.initial_gains is None:
        raise LiveSessionError("preflight gains are unavailable")
    baseline_gains = session.initial_gains[WHEELS.index(wheel)]
    await session.set_pid_checked(wheel, baseline_gains)
    await session.start_speed_loop()
    samples: list[dict] = []
    await session.run_stage(wheel, 0, 4.0, samples)
    await session.run_stage(wheel, 80, 12.0, samples)
    await session.run_stage(wheel, 0, 5.0, samples)
    await session.run_stage(wheel, 40, 7.0, samples)
    await session.run_stage(wheel, 0, 4.0, samples)
    await session.run_stage(wheel, 120, 7.0, samples)
    await session.run_stage(wheel, 0, 4.0, samples)
    await session.run_stage(wheel, 160, 7.0, samples)
    await session.run_stage(wheel, 0, 4.0, samples)

    baseline_path = output_dir / f"live-{wheel}-baseline.csv"
    save_csv(baseline_path, samples)
    gains, tuning = conservative_candidate(samples)
    applied = await session.set_pid_checked(wheel, gains)

    validation_samples: list[dict] = []
    await session.run_stage(wheel, 0, 3.0, validation_samples)
    await session.run_stage(wheel, 40, 7.0, validation_samples)
    await session.run_stage(wheel, 0, 3.0, validation_samples)
    await session.run_stage(wheel, 80, 7.0, validation_samples)
    await session.run_stage(wheel, 0, 3.0, validation_samples)
    await session.run_stage(wheel, 120, 7.0, validation_samples)
    await session.run_stage(wheel, 0, 3.0, validation_samples)
    await session.run_stage(wheel, 160, 7.0, validation_samples)
    await session.run_stage(wheel, 0, 3.0, validation_samples)
    validation_path = output_dir / f"live-{wheel}-validation.csv"
    save_csv(validation_path, validation_samples)

    report = {
        "wheel": wheel,
        "baseline_gains": baseline_gains,
        "applied_readback": applied,
        "tuning": tuning,
        "validation": evaluate(validation_samples, wheel),
        "files": [str(baseline_path), str(validation_path)],
    }
    (output_dir / f"live-{wheel}-report.json").write_text(
        json.dumps(report, indent=2), encoding="utf-8"
    )
    await session.command(f"SPD,{wheel},0")
    await session.command("SPEED,0")
    return report


async def main(url: str, output_dir: Path) -> int:
    output_dir.mkdir(parents=True, exist_ok=True)
    reports = []
    async with BridgeSession(url) as session:
        initial = await session.preflight()
        print(json.dumps({"preflight": initial}, indent=2))
        for wheel in WHEELS:
            print(f"START {wheel}")
            report = await tune_wheel(session, wheel, output_dir)
            reports.append(report)
            print(json.dumps(report, indent=2))
    (output_dir / "live-session-report.json").write_text(
        json.dumps(reports, indent=2), encoding="utf-8"
    )
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default="ws://127.0.0.1:8766")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "live-session-20261010",
    )
    args = parser.parse_args()
    try:
        raise SystemExit(asyncio.run(main(args.url, args.output_dir)))
    except KeyboardInterrupt:
        raise SystemExit(130)
