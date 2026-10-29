#!/usr/bin/env python3
"""Re-score saved raw HLOG experiments without bridge access or gain proposals."""
import argparse
import json
from dataclasses import asdict
from pathlib import Path

from attitude_tuner import FIRMWARE_CAPS
from closed_loop_tuning import STEP_SCORE_WEIGHTS
from hlog_experiment import score_logged


def replay(payload):
    if payload.get("error"):
        raise ValueError("source experiment recorded a failure: "+str(payload["error"]))
    if payload.get("scoring_source") != "downloaded_HLOG":
        raise ValueError("requires raw downloaded_HLOG provenance")
    firmware = payload.get("firmware")
    if firmware not in FIRMWARE_CAPS:
        raise ValueError("unsupported firmware profile")
    if payload.get("capabilities", FIRMWARE_CAPS[firmware]) != FIRMWARE_CAPS[firmware]:
        raise ValueError("firmware/CAPS mismatch")
    status = payload.get("status", [])
    records = payload.get("records", [])
    if (len(status) != 7 or status[:2] != ["HLOG", "STATUS"] or
            status[2] != "0" or status[5] != "0" or
            int(status[3]) != len(records) or int(status[6]) != len(records) or
            int(status[4]) != 1024 or not records):
        raise ValueError("invalid, active, empty or overwritten HLOG status")
    if firmware.endswith(("R18", "R19")) and any("tick_us" not in row for row in records):
        raise ValueError("microsecond timestamps required by firmware profile")
    phases = payload.get("phases")
    if not phases or len(phases) not in (2, 3):
        raise ValueError("original locked experiment phases required")
    _, result = score_logged(records, payload["loop"], phases)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, nargs="+", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    paths = sorted({p for source in args.input for p in
                    (source.glob("round-*-raw.json") if source.is_dir() else [source])})
    if not paths:
        parser.error("no raw HLOG experiments found")
    if args.output.exists():
        parser.error("output exists; choose a new report path")
    rows = []
    for path in paths:
        row = {"raw": str(path), "quality": "failed"}
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
            result = replay(payload)
            row.update(quality="passed", metrics=asdict(result),
                       loop=payload["loop"], gains=payload["gains"], phases=payload["phases"])
        except (ValueError, KeyError, TypeError, OSError) as exc:
            row["error"] = str(exc)
        rows.append(row)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"evidence": "offline_replay_of_saved_HLOG",
                                      "gain_proposals": False, "ranking": False,
                                      "weights": STEP_SCORE_WEIGHTS, "results": rows},
                                     indent=2, allow_nan=False), encoding="utf-8")
    print(f"Replayed {len(rows)} experiments; {sum(x['quality']=='passed' for x in rows)} passed. {args.output}")


if __name__ == "__main__":
    main()
