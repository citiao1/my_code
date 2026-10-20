"""RAM-HLOG direction-loop identification and conservative PD candidate search.

The model treats the already-closed angular-rate loop as the plant. Its input
is the outer-loop yaw-rate target and its measured output is yaw rate. Heading
is integrated from that measured rate for candidate validation.
"""
from __future__ import annotations

import csv
import math
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Iterable, Mapping

from closed_loop_tuning import wrap_deg


@dataclass(frozen=True)
class DirectionModel:
    sample_period_s: float
    delay_samples: int
    delay_s: float
    pole: float
    rate_gain: float
    tau_s: float
    rmse_dps: float
    samples: int


@dataclass(frozen=True)
class PDCandidate:
    kp: float
    kd: float
    score: float
    iae_deg_s: float
    overshoot_deg: float
    steady_error_deg: float
    saturation_fraction: float


def _finite_records(records: Iterable[Mapping[str, object]]):
    result = []
    for record in records:
        try:
            values = (
                float(record["target_rate_dps"]),
                float(record["actual_rate_dps"]),
            )
        except (KeyError, TypeError, ValueError):
            continue
        if all(math.isfinite(value) for value in values):
            result.append(values)
    return result


def _fit_arx(values, delay: int):
    # y[k+1] = a*y[k] + b*u[k-delay].
    sums = [0.0] * 5
    count = 0
    for index in range(delay, len(values) - 1):
        u = values[index - delay][0]
        y = values[index][1]
        next_y = values[index + 1][1]
        sums[0] += y * y
        sums[1] += y * u
        sums[2] += u * u
        sums[3] += y * next_y
        sums[4] += u * next_y
        count += 1
    determinant = sums[0] * sums[2] - sums[1] * sums[1]
    if count < 8 or abs(determinant) < 1e-9:
        return None
    pole = (sums[3] * sums[2] - sums[4] * sums[1]) / determinant
    coefficient = (sums[4] * sums[0] - sums[3] * sums[1]) / determinant
    residual = []
    for index in range(delay, len(values) - 1):
        prediction = pole * values[index][1] + coefficient * values[index - delay][0]
        residual.append(prediction - values[index + 1][1])
    rmse = math.sqrt(sum(error * error for error in residual) / len(residual))
    return pole, coefficient, rmse, count


def fit_direction_model(
    records: Iterable[Mapping[str, object]],
    *,
    sample_period_s: float = 0.02,
    max_delay_s: float = 0.30,
) -> DirectionModel:
    """Fit a stable delayed ARX approximation from HLOG records."""
    if not math.isfinite(sample_period_s) or sample_period_s <= 0:
        raise ValueError("sample period must be positive")
    if not math.isfinite(max_delay_s) or max_delay_s < 0:
        raise ValueError("maximum delay must be nonnegative")
    values = _finite_records(records)
    max_delay = max(0, int(max_delay_s / sample_period_s))
    fits = [
        (delay, _fit_arx(values, delay))
        for delay in range(max_delay + 1)
    ]
    stable = [
        (delay, fit) for delay, fit in fits if fit is not None and
        0.0 < fit[0] < 0.9999 and math.isfinite(fit[1]) and fit[1] > 0.0
    ]
    if not stable:
        raise ValueError("HLOG does not contain a stable rate response")
    delay, (pole, coefficient, rmse, count) = min(
        stable, key=lambda item: item[1][2]
    )
    gain = coefficient / max(1.0 - pole, 1e-6)
    tau = -sample_period_s / math.log(pole)
    return DirectionModel(
        sample_period_s=sample_period_s,
        delay_samples=delay,
        delay_s=delay * sample_period_s,
        pole=pole,
        rate_gain=gain,
        tau_s=tau,
        rmse_dps=rmse,
        samples=count,
    )


def load_hlog_csv(path: Path):
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def _simulate(
    model: DirectionModel,
    kp: float,
    kd: float,
    amplitude: float,
    *,
    dwell_s: float = 1.5,
    rate_limit_dps: float = 80.0,
):
    dt = model.sample_period_s
    steps_per_segment = max(4, int(dwell_s / dt))
    targets = [0.0, amplitude, 0.0, -amplitude, 0.0]
    rate = 0.0
    heading = 0.0
    queued = [0.0] * (model.delay_samples + 1)
    rows = []
    for target in targets:
        for _ in range(steps_per_segment):
            error = wrap_deg(target - heading)
            command = max(-rate_limit_dps, min(rate_limit_dps, kp * error - kd * rate))
            delayed = queued.pop(0)
            queued.append(command)
            rate = model.pole * rate + (1.0 - model.pole) * model.rate_gain * delayed
            heading = wrap_deg(heading + dt * rate)
            rows.append((target, heading, command, error))
    return rows


def search_direction_pd(
    model: DirectionModel,
    current: tuple[float, float],
    *,
    amplitudes=(5.0, 10.0, 15.0),
    change_limit=0.20,
) -> list[PDCandidate]:
    """Return conservative PD candidates ranked on multiple heading amplitudes."""
    kp, kd = current
    if not all(math.isfinite(value) and 0.0 <= value <= 100.0
               for value in current):
        raise ValueError("current gains must be finite and in 0..100")
    if not amplitudes or any(
        not math.isfinite(value) or value <= 0.0 or value > 45.0
        for value in amplitudes
    ):
        raise ValueError("amplitudes must be in 0..45 degrees")
    if not 0.0 < change_limit <= 0.20:
        raise ValueError("change limit must be in (0, 0.20]")
    p_values = sorted({round(max(0.0, min(100.0, kp * factor)), 4)
                       for factor in (1.0 - change_limit, 1.0,
                                      1.0 + change_limit)})
    d_values = sorted({round(max(0.0, min(100.0, kd * factor)), 4)
                       for factor in (1.0 - change_limit, 1.0,
                                      1.0 + change_limit)})
    candidates = []
    for candidate_kp in p_values:
        for candidate_kd in d_values:
            metrics = []
            for amplitude in amplitudes:
                rows = _simulate(model, candidate_kp, candidate_kd, amplitude)
                errors = [abs(row[3]) for row in rows]
                iae = sum(errors) * model.sample_period_s
                overshoot = max(
                    0.0,
                    max(abs(row[1]) for row in rows) - abs(amplitude),
                )
                steady = sum(errors[-max(4, len(errors) // 5):]) / max(
                    4, len(errors) // 5
                )
                saturated = sum(
                    abs(row[2]) >= 0.98 * 80.0 for row in rows
                ) / len(rows)
                metrics.append((iae, overshoot, steady, saturated))
            worst = tuple(max(metric[index] for metric in metrics)
                          for index in range(4))
            score = worst[0] / max(amplitudes) + 2.0 * worst[1] + \
                worst[2] + worst[3]
            candidates.append(PDCandidate(
                candidate_kp, candidate_kd, score, worst[0], worst[1],
                worst[2], worst[3],
            ))
    return sorted(candidates, key=lambda result: result.score)


def model_as_dict(model: DirectionModel) -> dict:
    return asdict(model)


def main() -> int:
    import argparse
    import json

    parser = argparse.ArgumentParser(
        description="Fit the closed angular-rate plant from a direction HLOG CSV",
    )
    parser.add_argument("hlog_csv", type=Path)
    parser.add_argument("--kp", type=float, default=5.0)
    parser.add_argument("--kd", type=float, default=1.25)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    model = fit_direction_model(load_hlog_csv(args.hlog_csv))
    candidates = search_direction_pd(model, (args.kp, args.kd))
    result = {
        "model": model_as_dict(model),
        "current_gains": [args.kp, args.kd],
        "candidates": [asdict(candidate) for candidate in candidates],
        "writeback": False,
        "validation_required": True,
    }
    text = json.dumps(result, indent=2)
    if args.output:
        args.output.write_text(text + "\n", encoding="utf-8")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

