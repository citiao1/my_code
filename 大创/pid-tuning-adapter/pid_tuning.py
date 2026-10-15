"""Dependency-light PID step-response identification and tuning helpers.

The tuner works on any SISO loop that can be represented as
time/setpoint/process/output samples.  It produces candidates; it never
writes parameters to a controller.
"""

from __future__ import annotations

import csv
import json
import math
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, Sequence


@dataclass(frozen=True)
class Sample:
    time_s: float
    setpoint: float
    process: float
    output: float


@dataclass(frozen=True)
class StepResponse:
    step_time_s: float
    input_step: float
    process_step: float
    process_gain: float
    dead_time_s: float
    time_constant_s: float
    baseline_process: float
    final_process: float
    saturated: bool


@dataclass(frozen=True)
class PidCandidate:
    mode: str
    form: str
    kp: float
    ki: float
    kd: float
    sample_time_s: float
    output_limit: float | None
    notes: tuple[str, ...]


_ALIASES = {
    "time_s": ("time_s", "time", "timestamp_s", "t"),
    "setpoint": ("setpoint_rpm", "setpoint_dps", "setpoint_deg", "setpoint", "target", "target_rpm", "reference"),
    "process": ("process_rpm", "process_dps", "process_deg", "process", "feedback", "actual", "actual_rpm", "measurement"),
    "output": ("output_percent", "output_rpm", "output_dps", "output", "control", "u", "command"),
}


def _find_column(fieldnames: Sequence[str], aliases: Sequence[str]) -> str:
    lower = {field.lower(): field for field in fieldnames}
    for alias in aliases:
        if alias in lower:
            return lower[alias]
    raise ValueError(f"missing required CSV column; expected one of {aliases}")


def load_csv(path: str | Path) -> list[Sample]:
    with Path(path).open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if not reader.fieldnames:
            raise ValueError("CSV has no header")
        columns = {
            name: _find_column(reader.fieldnames, aliases)
            for name, aliases in _ALIASES.items()
        }
        samples = []
        for row in reader:
            try:
                sample = Sample(*(float(row[columns[name]]) for name in _ALIASES))
            except (TypeError, ValueError, KeyError):
                continue
            if all(math.isfinite(value) for value in asdict(sample).values()):
                samples.append(sample)
    if len(samples) < 8:
        raise ValueError("at least 8 finite samples are required")
    if any(b.time_s <= a.time_s for a,b in zip(samples,samples[1:])):
        raise ValueError("CSV time must be strictly monotonic")
    return samples


def _median(values: Iterable[float]) -> float:
    ordered = sorted(values)
    middle = len(ordered) // 2
    if not ordered:
        raise ValueError("empty sample set")
    if len(ordered) % 2:
        return ordered[middle]
    return (ordered[middle - 1] + ordered[middle]) / 2.0


def _mean(values: Iterable[float]) -> float:
    values = list(values)
    return sum(values) / len(values)


def _first_step(samples: Sequence[Sample]) -> tuple[int, float, float]:
    span = max(sample.setpoint for sample in samples) - min(sample.setpoint for sample in samples)
    threshold = max(abs(span) * 0.05, 1e-9)
    baseline_count = max(3, len(samples) // 10)
    baseline = _median(sample.setpoint for sample in samples[:baseline_count])
    for index, sample in enumerate(samples):
        if abs(sample.setpoint - baseline) >= threshold:
            return index, baseline, sample.setpoint - baseline
    raise ValueError("no setpoint step detected")


def identify_fopdt(
    samples: Sequence[Sample],
    *,
    response_fraction: float = 0.632,
    saturation_fraction: float = 0.95,
    output_limit: float | None = None,
) -> StepResponse:
    """Identify from a genuine held actuator step, not a closed-loop reference."""
    if not 0.0 < response_fraction < 1.0:
        raise ValueError("response_fraction must be between 0 and 1")
    actuator_samples = [Sample(s.time_s, s.output, s.process, s.output) for s in samples]
    index, baseline_output, input_step = _first_step(actuator_samples)
    if index < 3:
        raise ValueError("actuator step requires pre-step samples")
    if any(abs(s.output-baseline_output-input_step) > abs(input_step)*0.03
           for s in samples[index:]):
        raise ValueError("varying closed-loop output: FOPDT requires a held actuator step; use closed-loop scoring")
    pre_count = max(3, min(index, len(samples) // 10))
    baseline_process = _median(sample.process for sample in samples[max(0, index - pre_count):index])
    tail_count = max(3, len(samples) // 10)
    final_process = _mean(sample.process for sample in samples[-tail_count:])
    process_step = final_process - baseline_process
    if abs(input_step) < 1e-12 or abs(process_step) < 1e-12:
        raise ValueError("step response has no measurable process change")
    process_gain = process_step / input_step
    sign = 1.0 if process_step >= 0.0 else -1.0
    first_response = baseline_process + sign * abs(process_step) * 0.02
    target_response = baseline_process + process_step * response_fraction
    dead_index = next(
        (i for i in range(index, len(samples)) if sign * (samples[i].process - baseline_process) >= sign * (first_response - baseline_process)),
        None,
    )
    tau_index = next(
        (i for i in range(index, len(samples)) if sign * (samples[i].process - target_response) >= 0.0),
        None,
    )
    if dead_index is None or tau_index is None:
        raise ValueError("response did not reach dead-time or time-constant threshold")
    step_time = samples[index].time_s
    dead_time = max(0.0, samples[dead_index].time_s - step_time)
    tau = max(0.0, samples[tau_index].time_s - step_time - dead_time)
    output_values = [sample.output for sample in samples[index:]]
    tail_values = output_values[-max(3, len(output_values) // 10):]
    output_peak = max(abs(value) for value in output_values) if output_values else 0.0
    tail_mean = _mean(tail_values) if tail_values else 0.0
    tail_range = (max(tail_values) - min(tail_values)) if tail_values else 0.0
    output_span = (max(output_values) - min(output_values)) if output_values else 0.0
    saturated = output_limit is not None and output_peak >= output_limit*saturation_fraction
    if abs(samples[-1].process-samples[-tail_count].process) > abs(process_step)*0.1:
        raise ValueError("plant has not settled or is integrating; FOPDT is not applicable")
    return StepResponse(
        step_time_s=step_time,
        input_step=input_step,
        process_step=process_step,
        process_gain=process_gain,
        dead_time_s=dead_time,
        time_constant_s=max(tau, 1e-9),
        baseline_process=baseline_process,
        final_process=final_process,
        saturated=saturated,
    )


def tune_fopdt(
    model: StepResponse,
    *,
    mode: str = "PI",
    form: str = "parallel",
    sample_time_s: float | None = None,
    closed_loop_time_s: float | None = None,
    output_limit: float | None = None,
) -> PidCandidate:
    """Return IMC/SIMC-style conservative gains for P/PI/PD/PID.

    ``parallel`` is continuous-time notation (Ki in 1/s, Kd in s).
    ``incremental`` converts those gains to the current STM32 discrete update:
    u += Kp*Δe + Ki_step*e + Kd_step*Δ²e.
    ``position-discrete`` uses Ki_step and Kd_step for a sum/error-difference
    implementation.  The output is a candidate only; saturation is reported.
    """
    mode = mode.upper()
    form = form.lower()
    if mode not in {"P", "PI", "PD", "PID"}:
        raise ValueError("mode must be P, PI, PD or PID")
    if form not in {"parallel", "incremental", "position-discrete"}:
        raise ValueError("unsupported PID form")
    if model.process_gain == 0.0:
        raise ValueError("process gain cannot be zero")
    sample_time_s = sample_time_s or 0.0
    if sample_time_s < 0.0:
        raise ValueError("sample_time_s cannot be negative")
    lam = closed_loop_time_s or max(5.0 * sample_time_s, model.dead_time_s, 1e-3)
    gain = model.process_gain
    tau = model.time_constant_s
    dead = model.dead_time_s
    kp = tau / (gain * (lam + dead))
    ti = tau
    td = (tau * dead) / max(2.0 * tau + dead, 1e-9)
    if mode == "P":
        ki, kd = 0.0, 0.0
    elif mode == "PI":
        ki, kd = kp / max(ti, 1e-9), 0.0
    elif mode == "PD":
        ki, kd = 0.0, kp * td
    else:
        ki, kd = kp / max(ti, 1e-9), kp * td
    notes = []
    if model.saturated:
        notes.append("identification data appears saturated; validate with an unsaturated step")
    if form == "incremental":
        if sample_time_s <= 0.0:
            raise ValueError("sample_time_s is required for incremental form")
        ki, kd = ki * sample_time_s, kd / sample_time_s
        notes.append("gains converted from continuous parallel form to discrete incremental coefficients")
    elif form == "position-discrete":
        if sample_time_s <= 0.0:
            raise ValueError("sample_time_s is required for position-discrete form")
        ki, kd = ki * sample_time_s, kd / sample_time_s
        notes.append("Ki/Kd are per-sample coefficients for a discrete position PID")
    notes.append("candidate is not written to firmware automatically")
    return PidCandidate(mode, form, kp, ki, kd, sample_time_s, output_limit, tuple(notes))


def write_report(path: str | Path, model: StepResponse, candidates: Sequence[PidCandidate]) -> None:
    payload = {
        "model": asdict(model),
        "candidates": [asdict(candidate) for candidate in candidates],
    }
    Path(path).write_text(json.dumps(payload, indent=2), encoding="utf-8")
