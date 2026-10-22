#!/usr/bin/env python3
"""R17/R18/R19 PI-rate / PD-heading tuning from stopped RAM-log downloads."""
from __future__ import annotations
import argparse
import asyncio
import csv
import json
import math
import time
from dataclasses import asdict
from pathlib import Path
import websockets
from closed_loop_tuning import score_tracking, wrap_deg
from campaign_runner import execute_campaign
from tuning_campaign import make_plan
from pid_tuning import Sample
from hlog_experiment import recording_budget, score_logged
from pid_ws_logger import (
    BRIDGE_GREETING, ENCODER_COUNTS_PER_REV,
    METERS_PER_ENCODER_COUNT, WHEEL_DIAMETER_M, distance_from_encoder,
    encoder_row, hlog_row, hlog_samples, speed_drive_row, telemetry_row,
)

FIRMWARE_VERSION = "VER,VGT6_BM50_ATTITUDE_R17"
CAPS = "CAPS,ATTITUDE_R17,20,10,160,60,60,80,SPEED_TUNABLE,HLOG,PS2,TRACKMOVE,HLOG_MODE"
FIRMWARE_CAPS = {
    FIRMWARE_VERSION: CAPS,
    FIRMWARE_VERSION.replace("R17", "R18"): CAPS.replace("R17", "R18") + ",HLOG_US",
    FIRMWARE_VERSION.replace("R17", "R19"): CAPS.replace("R17", "R19") + ",HLOG_US,TOF_DL1B",
    FIRMWARE_VERSION.replace("R17", "R20"): CAPS.replace("R17", "R20") + ",HLOG_US,TOF_DL1B,WLOG_US,TOF_GUARD,FRONT_REVERSED",
}
FIRMWARE_CAPS[FIRMWARE_VERSION.replace("R17", "R21")] = (
    FIRMWARE_CAPS[FIRMWARE_VERSION.replace("R17", "R20")].replace("R20", "R21") + ",TOF_FAST"
)
FIRMWARE_CAPS[FIRMWARE_VERSION.replace("R17", "R22")] = (
    FIRMWARE_CAPS[FIRMWARE_VERSION.replace("R17", "R21")].replace("R21", "R22") + ",TOF_GUARD_200_100_40"
)
FIRMWARE_CAPS[FIRMWARE_VERSION.replace("R17", "R23")] = (
    FIRMWARE_CAPS[FIRMWARE_VERSION.replace("R17", "R22")].replace("R22", "R23") + ",UART_QOS"
)
TARGET_WHEEL_LIMIT = 160
HEADING_FINAL_SETTLE_S = 1.0
HEADING_REVERSAL_PAUSE_S = 1.5
HEADING_TELEMETRY_MAX_AGE_S = 1.25
FIXED = [x for ki in (0.0991,0.0998,0.0998,0.1008) for x in (1.2,ki,0)]


class Session:
    def __init__(self, ws):
        self.ws, self.serial = ws, 0
        self.pending = {}
        self.frames = asyncio.Queue(maxsize=256)
        self.encoder_frames = asyncio.Queue(maxsize=64)
        self.speed_frames = asyncio.Queue(maxsize=256)
        self.hlog_frames = asyncio.Queue(maxsize=2048)
        self.fault = None
        self.armed = False
        self.allow_stopped_heartbeat = False
        self.hlog_reading = False
        self.require_hlog_us = False
        self.expected_caps = CAPS
        self.firmware_version = FIRMWARE_VERSION
        self.motion_active = False
        self.timeline = []
        self.monitor = []
        self.download = {"status": None, "records": [], "wire": []}
        self.reader = asyncio.create_task(self.receive())

    @staticmethod
    def enqueue_latest(queue, item):
        if queue.full():
            try:
                queue.get_nowait()
            except asyncio.QueueEmpty:
                pass
        try:
            queue.put_nowait(item)
        except asyncio.QueueFull:
            pass

    async def receive(self):
        try:
            async for text in self.ws:
                if not isinstance(text, str):
                    continue
                if text.startswith("{"):
                    reply = json.loads(text)
                    future = self.pending.get(reply.get("id"))
                    if future and not future.done():
                        if reply.get("ok"):
                            future.set_result(reply.get("reply", ""))
                        else:
                            future.set_exception(RuntimeError(reply.get("error")))
                elif text.startswith("ATEL,"):
                    self.monitor.append({"host_monotonic_s": time.monotonic(), "line": text.strip()})
                    self.enqueue_latest(
                        self.frames, (time.monotonic(), text.strip())
                    )
                elif text.startswith("ENC,"):
                    self.enqueue_latest(
                        self.encoder_frames, (time.monotonic(), text.strip())
                    )
                elif text.startswith("TEL,"):
                    self.enqueue_latest(
                        self.speed_frames, (time.monotonic(), text.strip())
                    )
                elif text.startswith("HLOG,"):
                    self.download["wire"].append(text.strip())
                    self.hlog_frames.put_nowait(text.strip())  # Overflow is a fault, not silent loss.
                elif text.startswith(("STATUS,0,", "ERR,")):
                    if not (
                        self.allow_stopped_heartbeat and
                        text.startswith("ERR,KEEP,STOPPED")
                    ):
                        self.fault = RuntimeError(text.strip())
        except Exception as exc:
            self.fault = exc
        finally:
            for future in self.pending.values():
                if not future.done():
                    future.set_exception(ConnectionError("bridge receiver stopped"))

    async def command(self, text, *, timeout=5):
        entry = {"command": text, "host_monotonic_s": time.monotonic()}
        self.timeline.append(entry)
        self.serial += 1
        number = self.serial
        future = asyncio.get_running_loop().create_future()
        self.pending[number] = future
        try:
            await self.ws.send(json.dumps({"type":"command","id":number,"command":text}))
            try:
                result = await asyncio.wait_for(future, timeout)
                entry["reply"] = result
                return result
            except asyncio.TimeoutError as exc:
                raise RuntimeError(f"command timeout: {text}") from exc
        finally:
            self.pending.pop(number, None)

    async def heartbeat(self):
        try:
            while True:
                if self.hlog_reading or not self.motion_active:
                    await asyncio.sleep(0.1)
                    continue
                await self.command("KEEP")
                await asyncio.sleep(0.4)
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            if not (
                self.allow_stopped_heartbeat and
                (
                    "ERR,KEEP,STOPPED" in str(exc) or
                    "cancelled by STOP/DISARM" in str(exc)
                )
            ):
                self.fault = exc

    async def health(self):
        if self.fault:
            raise self.fault
        if await self.command("CAPS") != self.expected_caps:
            raise RuntimeError("firmware capability or fixed limits changed")
        if await self.command("SPEEDLIMIT") != f"SPEEDLIMIT,{TARGET_WHEEL_LIMIT}":
            raise RuntimeError("wheel ceiling changed")
        stack = (await self.command("STACK")).split(",")
        if len(stack) != 5 or stack[4] != "1" or int(stack[3]) < 32:
            raise RuntimeError("stack health failure")
        uart = await self.command("UART")
        if uart != self.uart_baseline:
            raise RuntimeError("UART diagnostics changed")
        link = (await self.command("LINK")).split(",")
        if len(link) != 7 or link[3:6] != self.link_baseline:
            raise RuntimeError("serial drops/overflows/errors changed")
        wheel = (await self.command("PIDGET")).split(",")
        if len(wheel) != 13 or wheel[0] != "PID":
            raise RuntimeError("malformed wheel gains")
        if any(not math.isfinite(float(value)) or not 0.0 <= float(value) <= 100.0
               for value in wheel[1:]):
            raise RuntimeError("wheel gains out of range")
        baseline = getattr(self, "wheel_baseline", None)
        if baseline is not None and tuple(float(value) for value in wheel[1:]) != baseline:
            raise RuntimeError("wheel PID changed during attitude tuning")

    async def get_gains(self, loop):
        fields = (await self.command("YAWGET" if loop=="angular_velocity" else "HEADGET")).split(",")
        if loop == "angular_velocity":
            if len(fields)!=6 or fields[0]!="YAWPID" or fields[3:]!=["60","80","20"]:
                raise RuntimeError("unexpected rate form/limits/period")
        elif len(fields)!=6 or fields[0]!="HEADPID" or fields[3:]!=["80","20","D_MEAS"]:
            raise RuntimeError("unexpected heading derivative/limits/period")
        gains = tuple(float(x) for x in fields[1:3])
        if not all(math.isfinite(x) and 0<=x<=100 for x in gains):
            raise RuntimeError("invalid gains")
        return gains

    async def read_hlog(self, *, batch_size=8, command_timeout=20.0,
                        frame_timeout=5.0, loop="direction"):
        """Read the stopped MCU RAM log in bounded UART-sized batches."""
        if not isinstance(batch_size, int) or not 1 <= batch_size <= 8:
            raise ValueError("HLOG batch size must be 1..8")
        self.hlog_reading = True
        try:
            status = (await self.command(
                "HLOG,STATUS", timeout=command_timeout
            )).split(",")
            self.download["status"] = status
            if len(status) != 7 or status[:2] != ["HLOG", "STATUS"]:
                raise RuntimeError("invalid HLOG status")
            total = int(status[3])
            if (status[2] != "0" or status[5] != "0" or
                    not 0 < total <= int(status[4]) or int(status[6]) != total):
                raise RuntimeError("HLOG active, empty, overflowed or overwritten")
            records_by_sequence = {}
            self.download["batch_retries"] = []
            offset = 0
            while offset < total:
                requested = min(batch_size, total - offset)
                expected = set(range(offset, offset + requested))
                for attempt in range(3):
                    try:
                        reply = await self.command(
                            f"HLOG,READ,{offset},{requested}",
                            timeout=command_timeout,
                        )
                    except (RuntimeError, TimeoutError) as error:
                        if not isinstance(error, TimeoutError) and "timeout" not in str(error).lower():
                            raise
                        self.download["batch_retries"].append({
                            "offset": offset, "count": requested,
                            "attempt": attempt + 1, "error": str(error),
                        })
                        if attempt == 2:
                            raise
                        continue
                    end = reply.split(",")
                    if (len(end) != 5 or end[:2] != ["HLOG", "END"] or
                            not all(value.isdecimal() for value in end[2:]) or
                            int(end[2]) != offset):
                        self.download["batch_retries"].append({
                            "offset": offset, "count": requested,
                            "attempt": attempt + 1, "invalid_end": reply,
                        })
                        if attempt == 2:
                            raise RuntimeError("invalid HLOG end marker")
                        continue
                    if int(end[4]) != total:
                        raise RuntimeError("HLOG total changed during download")
                    returned = int(end[3])
                    if returned != requested:
                        raise RuntimeError("HLOG batch was truncated")
                    deadline = time.monotonic() + frame_timeout
                    while (not expected.issubset(records_by_sequence) or not self.hlog_frames.empty()) and time.monotonic() < deadline:
                        try:
                            line = await asyncio.wait_for(
                                self.hlog_frames.get(),
                                max(0.01, deadline - time.monotonic()),
                            )
                        except asyncio.TimeoutError:
                            break
                        record = hlog_row(line)
                        if record is None:
                            continue
                        if self.require_hlog_us and "tick_us" not in record:
                            self.download.setdefault("invalid_timestamp_frames", []).append(line)
                            continue
                        if (self.require_hlog_us and
                                ((int(record["tick_us"]) - int(record["tick"]) * 1000) & 0xffffffff) >= 1000):
                            self.download.setdefault("invalid_timestamp_frames", []).append(line)
                            continue
                        sequence = int(record["sequence"])
                        if not 0 <= sequence < total:
                            raise RuntimeError("HLOG sequence outside recorded range")
                        previous = records_by_sequence.get(sequence)
                        if previous is not None and previous != record:
                            raise RuntimeError("conflicting HLOG retransmission")
                        if previous is None:
                            records_by_sequence[sequence] = record
                            self.download["records"].append(record)
                    if expected.issubset(records_by_sequence):
                        break
                    self.download["batch_retries"].append({
                        "offset": offset,
                        "count": requested,
                        "attempt": attempt + 1,
                        "missing": sorted(expected - records_by_sequence.keys()),
                    })
                else:
                    raise RuntimeError("HLOG batch remained incomplete after retries")
                offset += returned
            records = [records_by_sequence[index] for index in range(total)]
            if self.require_hlog_us and any("tick_us" not in row for row in records):
                raise RuntimeError("R18 HLOG requires a microsecond timestamp on every record")
            hlog_samples(records, loop, max_interval_ms=25)
            return records
        finally:
            self.hlog_reading = False

    async def stop_and_confirm(self):
        self.motion_active = False
        if await self.command("STOP") != "ACK,STOP":
            raise RuntimeError("STOP not acknowledged")
        fields = (await self.command("ANGLESTATUS")).split(",")
        if len(fields) != 14 or fields[0] != "ATEL" or fields[2] != "0" or fields[12] != "0":
            raise RuntimeError("controller did not enter stopped state")

    async def collect_logged(self, loop, phases):
        """ATEL is only a safety monitor; return complete downloaded HLOG."""
        self.allow_stopped_heartbeat = True
        await self.stop_and_confirm()
        while not self.hlog_frames.empty():
            self.hlog_frames.get_nowait()
        self.download = {"status": None, "records": [], "wire": []}
        await self.command("HLOG,CLEAR")
        await self.command("HLOG,START")
        heartbeat = None
        try:
            await self.command(self.motion_command(loop, phases[0][0]))
            self.motion_active = True
            heartbeat = asyncio.create_task(self.heartbeat())
            first_tick = None
            for target, seconds in phases:
                _, first_tick = await self.collect(loop, target, seconds, first_tick)
        finally:
            self.motion_active = False
            if heartbeat:
                heartbeat.cancel()
                await asyncio.gather(heartbeat, return_exceptions=True)
            try:
                await self.stop_and_confirm()
            finally:
                await self.command("HLOG,STOP")
        return await self.read_hlog(loop=loop)

    async def collect_heading_round_trip_logged(
        self, distance_m, forward_rpm, heading_amplitudes, seconds,
        heading_reference_deg,
    ):
        """Run a heading trajectory and retrieve the MCU's 20 ms RAM log."""
        self.allow_stopped_heartbeat = True
        await self.stop_and_confirm()
        await self.command("HLOG,CLEAR")
        await self.command("HLOG,START")
        heartbeat = asyncio.create_task(self.heartbeat())
        try:
            samples = await self.collect_heading_round_trip(
                distance_m, forward_rpm, heading_amplitudes, seconds,
                heading_reference_deg,
            )
        finally:
            self.motion_active = False
            heartbeat.cancel()
            await asyncio.gather(heartbeat, return_exceptions=True)
            try:
                await self.stop_and_confirm()
            finally:
                await self.command("HLOG,STOP")
        records = await self.read_hlog()
        return samples, records

    @staticmethod
    def motion_command(loop, target):
        if loop == "angular_velocity":
            # Match the console's Q/E path: MOVE enters the R9 rate cascade.
            return f"MOVE,0,0,{target:.4f}"
        return f"HEAD,{target:.4f}"

    async def write(self, loop, gains):
        if (loop not in ("angular_velocity", "direction") or len(gains) != 2 or
                any(not math.isfinite(x) or not 0 <= x <= 100 for x in gains)):
            raise ValueError("write requires two finite attitude gains in 0..100")
        stopped = (await self.command("ANGLESTATUS")).split(",")
        if (self.motion_active or len(stopped) != 14 or stopped[0] != "ATEL" or
                stopped[2] != "0" or stopped[12] != "0"):
            raise RuntimeError("PID writes require a verified stopped controller")
        old = await self.get_gains(loop)
        if any(abs(new-prev)>max(abs(prev)*0.2001,0.0001) for new,prev in zip(gains,old)):
            raise RuntimeError("per-write gain change exceeds locked 20% bound")
        name = "YAWPID" if loop=="angular_velocity" else "HEADPID"
        reply = await self.command(f"{name},{gains[0]:.4f},{gains[1]:.4f}")
        if reply != f"ACK,{name}" or await self.get_gains(loop)!=gains:
            raise RuntimeError("PID write/readback mismatch")

    async def collect(self, loop, target, seconds, first_tick):
        while not self.frames.empty():
            self.frames.get_nowait()
        await self.command(self.motion_command(loop, target))
        deadline = time.monotonic()+seconds
        samples=[]
        last_time = None
        while time.monotonic()<deadline:
            if self.fault:
                raise self.fault
            received, text = await asyncio.wait_for(self.frames.get(), 1.0)
            if time.monotonic()-received > 0.8:
                raise RuntimeError("stale telemetry")
            parsed = telemetry_row(text, None, first_tick, loop)
            if parsed is None:
                raise RuntimeError("IMU/control disabled or invalid ATEL")
            row, first_tick = parsed
            if abs(wrap_deg(row[1]-target) if loop=="direction" else row[1]-target) > 0.001:
                continue # one pre-command frame may already be queued in the UART
            fields = text.split(",")
            if abs(float(fields[5])/1000)>120:
                raise RuntimeError("rate outside session guard")
            if last_time is not None and not 0 < row[0]-last_time <= 0.8:
                raise RuntimeError("telemetry gap or non-monotonic tick")
            last_time = row[0]
            samples.append(Sample(*row))
        return samples, first_tick

    async def read_heading_reference(self):
        message = await self.command("ANGLESTATUS")
        fields = message.strip().split(",")
        if len(fields) != 14 or fields[0] != "ATEL":
            raise RuntimeError("heading reference telemetry unavailable")
        if int(fields[3]) != 1:
            raise RuntimeError("IMU is not ready for heading reference")
        return float(fields[8]) / 1000.0

    async def collect_heading_round_trip(
        self, distance_m, forward_rpm, heading_amplitudes, seconds,
        heading_reference_deg
    ):
        """Drive continuously through heading segments, then reverse the route."""
        if distance_m <= 0 or forward_rpm == 0:
            raise ValueError("distance and forward speed must be nonzero")
        while not self.frames.empty():
            self.frames.get_nowait()
        await self.command("ENCZERO")
        base_heading = wrap_deg(heading_reference_deg)
        first_tick = None
        samples = []
        started = time.monotonic()

        async def read_sample():
            nonlocal first_tick
            while True:
                if self.fault:
                    raise self.fault
                received, text = await asyncio.wait_for(self.frames.get(), 1.0)
                if time.monotonic() - received > HEADING_TELEMETRY_MAX_AGE_S:
                    raise RuntimeError("stale telemetry")
                parsed = telemetry_row(text, None, first_tick, "direction")
                if parsed is not None:
                    row, first_tick = parsed
                    samples.append(Sample(*row))
                    return
                # ATEL can contain one or two mode=0 frames queued between
                # STOP/HLOG start and the first active heading frame.
                if time.monotonic() - started >= seconds:
                    raise RuntimeError("IMU/control disabled or invalid ATEL")

        async def run_segment(heading, speed):
            if time.monotonic() - started >= seconds:
                raise RuntimeError("heading trajectory exceeded test duration")
            while not self.frames.empty():
                self.frames.get_nowait()
            origin_packet = encoder_row(await self.command("ENC"))
            if origin_packet is None or origin_packet[1] != 1:
                raise RuntimeError("encoder feedback unavailable")
            origin = origin_packet[3]
            await self.command(f"HEADMOVE,{heading:.4f},{speed},0")
            self.motion_active = True
            segment_samples = 0
            while time.monotonic() - started < seconds:
                await read_sample()
                segment_samples += 1
                if segment_samples % 2:
                    continue
                enc = encoder_row(await self.command("ENC"))
                if enc is None or enc[1] != 1:
                    raise RuntimeError("encoder feedback unavailable")
                if distance_from_encoder(enc[3], origin) >= distance_m / len(heading_amplitudes):
                    return
            raise RuntimeError("heading trajectory exceeded test duration")

        forward_headings = [
            wrap_deg(heading_reference_deg + amplitude)
            for amplitude in heading_amplitudes
        ]
        for heading in forward_headings:
            await run_segment(heading, abs(forward_rpm))

        await self.command(f"HEADMOVE,{base_heading:.4f},0,0")
        await asyncio.sleep(HEADING_REVERSAL_PAUSE_S)
        while not self.frames.empty():
            self.frames.get_nowait()

        for heading in reversed(forward_headings):
            await run_segment(heading, -abs(forward_rpm))

        await self.command(f"HEADMOVE,{base_heading:.4f},0,0")
        settle_deadline = time.monotonic() + HEADING_FINAL_SETTLE_S
        while time.monotonic() < settle_deadline:
            await read_sample()
        if not samples:
            raise RuntimeError("heading trajectory produced no telemetry")
        return samples

    async def collect_ground_speed(self, distance_m, target_rpm, seconds):
        """Run a forward/backward measured-distance speed-loop validation."""
        if distance_m <= 0 or target_rpm == 0:
            raise ValueError("distance and target speed must be nonzero")
        await self.command("ENCZERO")
        origin_packet = encoder_row(await self.command("ENC"))
        if origin_packet is None or origin_packet[1] != 1:
            raise RuntimeError("encoder feedback unavailable")
        origin = origin_packet[3]
        first_tick = None
        samples = []
        started = time.monotonic()
        next_speed_poll = started
        next_encoder_poll = started
        leg = 0
        await self.command(f"MOVE,{abs(int(target_rpm))},0,0")
        self.motion_active = True
        # KEEP is only valid after MOVE has enabled the attitude controller.
        # Starting it immediately after ARM would receive ERR,KEEP,STOPPED
        # and leave the later motion without watchdog heartbeats.
        heartbeat = asyncio.create_task(self.heartbeat())
        try:
            while time.monotonic() - started < seconds:
                now = time.monotonic()
                if now < min(next_speed_poll, next_encoder_poll):
                    await asyncio.sleep(
                        max(0.01, min(next_speed_poll, next_encoder_poll) - now)
                    )
                    continue
                if now >= next_speed_poll:
                    # MOVE follows the console path and selects ATEL. A
                    # one-shot SPEEDSTATUS query preserves that control path
                    # without racing a TEL stream switch during reversal.
                    parsed = speed_drive_row(
                        await self.command("SPEEDSTATUS"), first_tick,
                        front_reversed=getattr(self, 'firmware_version', '').endswith(('R20', 'R21', 'R22', 'R23'))
                    )
                    next_speed_poll = time.monotonic() + 0.25
                    if parsed is not None:
                        row, first_tick = parsed
                        samples.append(Sample(*row))
                if time.monotonic() < next_encoder_poll:
                    continue
                enc = encoder_row(await self.command("ENC"))
                if enc is None or enc[1] != 1:
                    raise RuntimeError("encoder feedback unavailable")
                # The bridge serializes MCU commands globally. Keep probes
                # sparse enough that KEEP remains ahead of the 1.5 s watchdog.
                next_encoder_poll = time.monotonic() + 1.0
                if distance_from_encoder(enc[3], origin) < distance_m:
                    continue
                if leg == 0:
                    leg = 1
                    origin = enc[3]
                    # Reverse before any other motion/telemetry command.
                    await self.command(f"MOVE,{-abs(int(target_rpm))},0,0")
                    next_encoder_poll = time.monotonic() + 1.0
                else:
                    break
        finally:
            heartbeat.cancel()
            await asyncio.gather(heartbeat, return_exceptions=True)
            for command in ("STOP", "TEL,0"):
                try:
                    await self.command(command)
                except Exception:
                    pass
        if leg != 1:
            raise RuntimeError("ground speed round trip did not complete")
        return samples


async def run(args):
    budget = recording_budget(args)  # Reject oversized/unsupported runs before ARM.
    args.output.mkdir(parents=True, exist_ok=True)
    if (any(args.output.glob("round-*-raw.json")) or
            (args.output / "session.json").exists()):
        raise ValueError("output already contains trial data; choose a new session directory")
    async with websockets.connect(args.url, open_timeout=5) as ws:
        if (await asyncio.wait_for(ws.recv(),5)).strip()!=BRIDGE_GREETING:
            raise RuntimeError("R3 bridge required")
        s = Session(ws)
        try:
            version = await s.command("VER")
            if version not in FIRMWARE_CAPS:
                raise RuntimeError("supported R17/R18/R19 firmware with mode-tagged 50 Hz HLOG required")
            s.require_hlog_us = version.endswith(("R18", "R19", "R20", "R21", "R22", "R23"))
            s.expected_caps = FIRMWARE_CAPS[version]
            s.firmware_version = version
            s.uart_baseline = await s.command("UART")
            u = s.uart_baseline.split(",")
            if len(u)!=10 or u[0]!="UART" or any(int(x) for x in u[5:]):
                raise RuntimeError("UART fault counters nonzero before experiment")
            link=(await s.command("LINK")).split(",")
            if len(link)!=7: raise RuntimeError("invalid LINK")
            s.link_baseline=link[3:6]
            await s.health()
            experiment = getattr(args, "experiment", "step")
            if experiment == "ground_speed":
                if not args.live:
                    print(json.dumps({
                        "capabilities": s.expected_caps,
                        "mode": "ground_speed_round_trip",
                        "wheel_diameter_m": WHEEL_DIAMETER_M,
                        "encoder_counts_per_rev": ENCODER_COUNTS_PER_REV,
                        "motion_started": False,
                    }, indent=2))
                    return
                if not args.confirm_hardware:
                    answer = input(
                        "首次确认：车体已落地、前后各有安全距离、急停有效，"
                        f"往返 {args.distance_m:.3f} m、目标 {args.speed_rpm:.1f} RPM。输入 YES：")
                    if answer != "YES":
                        raise RuntimeError("hardware not confirmed")
                await s.command("ARM")
                s.armed = True
                samples = await s.collect_ground_speed(
                    args.distance_m, args.speed_rpm, args.seconds)
                result = score_tracking(
                    samples, angle=False, output_limit=60, max_interval_s=1.25,
                    min_samples=4,
                )
                csv_path = args.output / "ground-speed-round-trip.csv"
                with csv_path.open("w", newline="", encoding="utf-8") as stream:
                    writer = csv.writer(stream)
                    writer.writerow(("time_s", "setpoint_rpm", "process_rpm",
                                     "output_percent"))
                    writer.writerows((x.time_s, x.setpoint, x.process, x.output)
                                     for x in samples)
                (args.output / "ground-speed-session.json").write_text(
                    json.dumps({
                        "mode": "ground_speed_round_trip",
                        "distance_m": args.distance_m,
                        "speed_rpm": args.speed_rpm,
                        "wheel_diameter_m": WHEEL_DIAMETER_M,
                        "encoder_counts_per_rev": ENCODER_COUNTS_PER_REV,
                        "metrics": asdict(result),
                        "csv": str(csv_path),
                    }, indent=2), encoding="utf-8")
                print(json.dumps(asdict(result), indent=2))
                return
            current=await s.get_gains(args.loop)
            inner=await s.get_gains("angular_velocity")
            wheel_fields = (await s.command("PIDGET")).split(",")
            s.wheel_baseline = tuple(float(value) for value in wheel_fields[1:])
            status = (await s.command("HLOG,STATUS")).split(",")
            if (len(status) != 7 or status[:2] != ["HLOG", "STATUS"] or
                    status[2] != "0" or int(status[4]) != 1024):
                raise RuntimeError("HLOG capacity/idle preflight failed")
            plan = make_plan(args, current) if experiment in ("step", "spin") else None
            if not args.live:
                print(json.dumps({"capabilities":s.expected_caps,"gains":current,"inner":inner,
                                  "scoring_source":"downloaded_HLOG",
                                  "recording_budget_s":budget,
                                  "campaign_plan":asdict(plan) if plan else None,
                                  "motion_started":False},indent=2))
                return
            if not args.confirm_hardware:
                answer=input("首次确认：车体可实际旋转、场地清空、急停有效，"
                             f"训练目标 {args.baseline}→{args.target}，"
                             f"留出目标 {plan.validation_targets if plan else '无写回'}，"
                             f"每项重复 {plan.repeats if plan else 1} 次，"
                             "固定限幅 60 RPM / 80°/s。输入 YES：")
                if answer!="YES": raise RuntimeError("hardware not confirmed")
            # One confirmation/ARM, but stop for every download and gain write.
            await s.command("ARM")
            s.armed=True
            await s.command("ATEL,1")
            heading_reference = None
            if args.loop == "direction":
                heading_reference = await s.read_heading_reference()
            if plan is not None:
                s.campaign_active = True
                report = await execute_campaign(args, s, plan, inner, heading_reference, save_round)
                print(json.dumps({"status": report["status"], "selection": report["selection"],
                                  "best_gains": report["best_gains"],
                                  "best_score": report["best_score"],
                                  "experiments": len(report["history"]),
                                  "session": str(args.output / "session.json")}, indent=2))
                return
            best_gains=current
            best_score=float("inf")
            history=[]
            for round_index in range(args.rounds):
                await s.health()
                if await s.get_gains(args.loop)!=current:
                    raise RuntimeError("external PID change")
                if args.loop=="direction" and await s.get_gains("angular_velocity")!=inner:
                    raise RuntimeError("inner rate gains changed while tuning outer loop")
                hlog_records = None
                if experiment == "direction_identify":
                    measured, hlog_records = await s.collect_heading_round_trip_logged(
                        args.distance_m, args.forward_rpm,
                        [0.0, args.target, 0.0, -args.target, 0.0],
                        args.seconds, heading_reference)
                    # Low-rate ATEL above is only the live safety/route monitor.
                    # Identification metrics must use downloaded 20 ms records.
                    measured = [Sample(*row) for row in hlog_samples(hlog_records)]
                    save_round(args, round_index, s, current, hlog_records)
                    result = score_tracking(
                        measured,
                        angle=True,
                        output_limit=80,
                        max_interval_s=HEADING_REVERSAL_PAUSE_S + 1.0,
                    )
                else:
                    offset = heading_reference if args.loop == "direction" else 0.0
                    convert = wrap_deg if args.loop == "direction" else lambda x: x
                    phases = [(convert(offset + args.baseline), args.baseline_seconds),
                              (convert(offset + args.target), args.seconds)]
                    if experiment == "spin":
                        phases[1] = (abs(args.target), args.seconds)
                        phases.append((-abs(args.target), args.seconds))
                    hlog_records = await s.collect_logged(args.loop, phases)
                    save_round(args, round_index, s, current, hlog_records, phases=phases)
                    measured, result = score_logged(hlog_records, args.loop, phases)
                await s.health()
                if await s.get_gains(args.loop)!=current:
                    raise RuntimeError("external PID change during measurement")
                if args.loop=="direction" and await s.get_gains("angular_velocity")!=inner:
                    raise RuntimeError("inner PID changed during outer-loop measurement")
                csv_path=args.output/f"round-{round_index+1:02d}.csv"
                with csv_path.open("w",newline="",encoding="utf-8") as stream:
                    writer=csv.writer(stream)
                    writer.writerow(("time_s","setpoint","process","output"))
                    writer.writerows((x.time_s,x.setpoint,x.process,x.output)
                                     for x in measured)
                hlog_path = None
                if hlog_records is not None:
                    hlog_path = args.output / f"round-{round_index+1:02d}-hlog.csv"
                    with hlog_path.open("w", newline="", encoding="utf-8") as stream:
                        writer = csv.DictWriter(stream, fieldnames=(
                            "sequence", "tick", "target_heading_deg",
                            "actual_heading_deg", "heading_error_deg",
                            "target_rate_dps", "actual_rate_dps",
                            "heading_output_dps", "rate_output_rpm",
                            "saturated", "forward_rpm", "left_rpm", "mode", "tick_us",
                        ))
                        writer.writeheader()
                        writer.writerows(hlog_records)
                if result.score<best_score:
                    best_score,best_gains=result.score,current
                if experiment == "direction_identify":
                    record={"round":round_index+1,"gains":current,
                            "metrics":asdict(result),"csv":str(csv_path),
                            "hlog_csv":str(hlog_path),
                            "writeback":False}
                    history.append(record)
                    (args.output/"session.json").write_text(json.dumps({
                        "loop":"direction","experiment":"direction_identify",
                        "form":"parallel","period_s":0.02,
                        "derivative":"measurement","telemetry_period_s":0.2,
                        "scoring_source":"downloaded_HLOG", "log_period_s":0.02,
                        "limits":{"rpm":60,"dps":80,
                                  "wheel_rpm":TARGET_WHEEL_LIMIT},
                        "baseline":args.baseline,"target":args.target,
                        "forward_rpm":args.forward_rpm,
                        "distance_m":args.distance_m,"history":history,
                        "gains":current,"writeback":False,
                    }, indent=2), encoding="utf-8")
                    print(json.dumps(record))
                    continue
                raise RuntimeError("unsupported automatic experiment")
        except Exception as exc:
            if not getattr(s, "campaign_active", False):
                save_round(args, locals().get("round_index", 0), s,
                           locals().get("current"), s.download["records"], error=str(exc))
            raise
        finally:
            if s.armed:
                try: await s.command("STOP")
                except Exception as exc: print(f"STOP unavailable: {exc}; MCU heartbeat watchdog expires in 1.5s")
            s.reader.cancel()
            await asyncio.gather(s.reader,return_exceptions=True)


def save_round(args, index, session, gains, records, *, phases=None, error=None):
    """Persist the original records/diagnostics before any scoring or gain write."""
    path = args.output / f"round-{index+1:02d}-raw.json"
    previous = json.loads(path.read_text(encoding="utf-8")) if path.exists() else {}
    payload = {**previous, "firmware": session.firmware_version,
               "capabilities": session.expected_caps, "loop": args.loop,
               "gains": gains, "scoring_source": "downloaded_HLOG",
               "status": session.download["status"], "records": records,
               "wire": session.download["wire"], "timeline": session.timeline,
               "timestamp_unit": "us" if session.require_hlog_us else "ms",
               "batch_retries": session.download.get("batch_retries", []),
               "safety_monitor": session.monitor,
               "limits": {"wheel_rpm": 160, "rotation_rpm": 60, "rate_dps": 80},
               "error": error}
    if phases is not None:
        payload["phases"] = phases
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--loop",required=True,
                   choices=("angular_velocity","direction","wheel_speed"))
    p.add_argument("--url",default="ws://127.0.0.1:8766")
    p.add_argument("--live",action="store_true")
    p.add_argument("--confirm-hardware",action="store_true")
    p.add_argument("--baseline",type=float,default=0)
    p.add_argument("--target",type=float,default=15.0)
    p.add_argument("--rounds",type=int,default=5)
    p.add_argument("--search", choices=("bo", "de", "heuristic"), default="bo",
                   help="bounded training search; all methods require independent holdout")
    p.add_argument("--repeats", type=int, default=2)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--kp-bounds", type=float, nargs=2, metavar=("MIN", "MAX"))
    p.add_argument("--second-bounds", type=float, nargs=2, metavar=("MIN", "MAX"),
                   help="Ki for rate PI; Kd for heading PD")
    p.add_argument("--validation-targets", type=float, nargs="+",
                   help="held-out steps, fixed before ARM; default uses reverse and another amplitude")
    p.add_argument("--min-improvement", type=float, default=0.01)
    p.add_argument("--max-overshoot", type=float, default=0.30)
    p.add_argument("--seconds",type=float,default=6)
    p.add_argument("--baseline-seconds",type=float,default=4)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--experiment", choices=("step", "spin",
                   "heading_round_trip", "direction_identify",
                   "ground_speed"), default="step")
    p.add_argument("--distance-m", type=float, default=1.0)
    p.add_argument("--speed-rpm", type=float, default=30.0)
    p.add_argument("--forward-rpm", type=int, default=30)
    a=p.parse_args()
    if a.experiment == "ground_speed":
        if a.loop != "wheel_speed":
            p.error("ground_speed requires --loop wheel_speed")
        if not (0.2 <= a.distance_m <= 10.0 and 5 <= abs(a.speed_rpm) <= 160):
            p.error("ground speed distance must be 0.2..10m and speed 5..160 RPM")
        if not 10 <= a.seconds <= 180:
            p.error("ground speed duration must be 10..180s")
    elif a.loop == "wheel_speed":
        p.error("wheel_speed requires --experiment ground_speed")
    if a.experiment == "heading_round_trip" and a.loop != "direction":
        p.error("heading_round_trip requires --loop direction")
    if a.experiment == "direction_identify" and a.loop != "direction":
        p.error("direction_identify requires --loop direction")
    if a.experiment == "spin" and a.loop != "angular_velocity":
        p.error("spin requires --loop angular_velocity")
    bounds=80 if a.loop=="angular_velocity" else 180
    if not all(math.isfinite(x) for x in (a.baseline,a.target,a.seconds,a.baseline_seconds)):
        p.error("all numeric arguments must be finite")
    if a.experiment != "ground_speed" and (
        max(abs(a.baseline),abs(a.target))>bounds or not 1<=a.rounds<=20):
        p.error("invalid target or rounds")
    step=abs(wrap_deg(a.target-a.baseline) if a.loop=="direction" else a.target-a.baseline)
    if a.experiment != "ground_speed" and (
        not 5<=step<=60 or not 3<=a.seconds<=120 or not 3<=a.baseline_seconds<=20):
        p.error("step must be 5..60 in loop units; durations 3..120s")
    try: asyncio.run(run(a))
    except KeyboardInterrupt: return 130
    except Exception as exc:
        print(f"Tuning stopped: {exc!r}")
        return 1
    return 0


if __name__=="__main__":
    raise SystemExit(main())
