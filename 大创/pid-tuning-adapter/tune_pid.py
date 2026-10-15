#!/usr/bin/env python3
"""Offline PID candidate generator for SISO step-response CSV files."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from pid_tuning import identify_fopdt, load_csv, tune_fopdt, write_report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--modes", default="P,PI,PD,PID")
    parser.add_argument("--forms", default="parallel,incremental,position-discrete")
    parser.add_argument("--sample-time", type=float, default=None)
    parser.add_argument("--closed-loop-time", type=float, default=None)
    parser.add_argument("--output-limit", type=float, default=None)
    args = parser.parse_args()

    samples = load_csv(args.input)
    model = identify_fopdt(samples, output_limit=args.output_limit)
    candidates = [
        tune_fopdt(
            model,
            mode=mode.strip(),
            form=form.strip(),
            sample_time_s=args.sample_time,
            closed_loop_time_s=args.closed_loop_time,
            output_limit=args.output_limit,
        )
        for mode in args.modes.split(",")
        for form in args.forms.split(",")
    ]
    write_report(args.output, model, candidates)
    print(json.dumps({"model": model.__dict__, "candidates": [candidate.__dict__ for candidate in candidates]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
