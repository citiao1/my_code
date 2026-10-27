"""Stopped-only serial latency/load probe. Never clears RAM logs or arms motors."""
import argparse
import collections
import json
import math
import statistics
import time
from pathlib import Path

import serial


def summary(values):
    values = sorted(values)
    return {"count": len(values), "median_ms": statistics.median(values),
            "p95_ms": values[math.ceil(len(values)*.95)-1], "max_ms": values[-1]} if values else {}


def run(args):
    report = {"port": args.port, "baud": 115200, "transport": "raw_serial",
              "query_cooldown_s": args.interval, "stop_every": args.stop_every,
              "commands": [], "chunks": [], "phases": []}
    buffer = b""
    with serial.Serial(args.port, 115200, timeout=.02, write_timeout=1) as port:
        def command(text, expected):
            nonlocal buffer
            entry = {"command": text, "start_s": time.monotonic(), "lines": []}
            report["commands"].append(entry)
            payload = ("\n" + text + "\n").encode("ascii")
            if port.write(payload) != len(payload):
                raise RuntimeError("short serial write")
            entry["write_done_s"] = time.monotonic()
            while time.monotonic() - entry["start_s"] < 4:
                while b"\n" in buffer:
                    line, buffer = buffer.split(b"\n", 1)
                    line = line.decode("ascii", "strict").strip()
                    entry["lines"].append(line)
                    if line == expected or line.startswith(expected + ","):
                        entry["reply"] = line
                        entry["rtt_ms"] = round((time.monotonic()-entry["start_s"])*1000, 3)
                        return line
                    if line.startswith("ERR,"):
                        raise RuntimeError(line)
                data = port.read(max(1, min(port.in_waiting, 4096)))
                if data:
                    now = time.monotonic()
                    entry.setdefault("first_chunk_s", now)
                    report["chunks"].append({"t_s": now, "hex": data.hex(), "bytes": len(data)})
                    buffer += data
            raise TimeoutError(text)

        try:
            for cmd, expected in [("STOP", "ACK,STOP"), ("DISARM", "ACK,DISARM"),
                                  ("TEL,0", "ACK,TEL,0"), ("TOF,0", "ACK,TOF,0"),
                                  ("VER", "VER"), ("CAPS", "CAPS"), ("UART", "UART"),
                                  ("LINK", "LINK"), ("STACK", "STACK"),
                                  ("SPEEDSTATUS", "TEL"), ("ANGLESTATUS", "ATEL")]:
                command(cmd, expected)
            loads = {"off": (0, 0), "tof": (0, 1), "tel": (1, 0), "both": (1, 1),
                     "atel": (1, 0), "atel_tof": (1, 1)}
            for name in args.phases:
                tel, tof = loads[name]
                stream = "ATEL" if name.startswith("atel") else "TEL"
                command(f"{stream},{tel}", f"ACK,{stream},{tel}")
                command(f"TOF,{tof}", f"ACK,TOF,{tof}")
                start = time.monotonic()
                first = len(report["commands"])
                chunk_first = len(report["chunks"])
                for index in range(args.samples):
                    command("PING", "PONG")
                    if args.stop_every and (index + 1) % args.stop_every == 0:
                        command("STOP", "ACK,STOP")
                    time.sleep(args.interval)
                elapsed = time.monotonic()-start
                entries = report["commands"][first:]
                counts = collections.Counter(line.split(",")[0] for e in entries for line in e["lines"])
                phase = {"name": name, "elapsed_s": elapsed,
                         "summary": summary([e["rtt_ms"] for e in entries if e["command"] == "PING"]),
                         "stop_summary": summary([e["rtt_ms"] for e in entries if e["command"] == "STOP"]),
                         "line_counts": dict(counts),
                         "rx_bytes_per_s": sum(c["bytes"] for c in report["chunks"][chunk_first:])/elapsed}
                report["phases"].append(phase)
                print(json.dumps(phase), flush=True)
                for cmd in ("UART", "LINK", "LINKQ", "TOFINFO"):
                    command(cmd, cmd)
        except Exception as exc:
            report["error"] = repr(exc)
            raise
        finally:
            for cmd, expected in [("STOP", "ACK,STOP"), ("DISARM", "ACK,DISARM"),
                                  ("TEL,0", "ACK,TEL,0"), ("TOF,0", "ACK,TOF,0"),
                                  ("SPEEDSTATUS", "TEL"), ("ANGLESTATUS", "ATEL")]:
                try:
                    command(cmd, expected)
                except Exception as exc:
                    report.setdefault("cleanup_errors", []).append(repr(exc))
            args.output.write_text(json.dumps(report, indent=2), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--samples", type=int, default=24)
    parser.add_argument("--interval", type=float, default=.12, help="idle seconds between requests")
    parser.add_argument("--stop-every", type=int, default=0, help="also measure a safe STOP every N PINGs")
    parser.add_argument("--phases", nargs="+", choices=("off", "tof", "tel", "both", "atel", "atel_tof"),
                        default=["off", "tof", "tel", "both"])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if (args.samples < 1 or not math.isfinite(args.interval) or args.interval < 0 or
            args.stop_every < 0 or args.output.exists()):
        parser.error("use positive samples, nonnegative interval/stop-every, and a fresh output file")
    run(args)
