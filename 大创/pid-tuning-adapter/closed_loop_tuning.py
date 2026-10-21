"""Score measured closed-loop steps; never mistake reference for actuator input."""
from __future__ import annotations
import math
from dataclasses import asdict, dataclass
from statistics import mean, median
from pid_tuning import Sample


# Persist this definition with each campaign so scores remain comparable.
STEP_SCORE_WEIGHTS = {
    "normalized_iae": 1.0, "overshoot": 2.0, "steady_error_rms": 1.0,
    "normalized_settling": 0.5, "saturation_fraction": 0.1,
}


def wrap_deg(x: float) -> float:
    return (x + 180.0) % 360.0 - 180.0


@dataclass(frozen=True)
class ResponseScore:
    normalized_iae: float
    overshoot: float
    steady_error: float
    settling_s: float | None
    saturation_fraction: float
    resolved_transient: bool
    score: float


def score_bidirectional(
    positive: list[Sample],
    negative: list[Sample],
    *,
    output_limit: float,
    angle: bool = False,
) -> ResponseScore:
    """Score matched positive/negative closed-loop tests as one experiment."""
    first = score_step(positive, angle=angle, output_limit=output_limit)
    second = score_step(negative, angle=angle, output_limit=output_limit)
    return ResponseScore(
        normalized_iae=(first.normalized_iae + second.normalized_iae) / 2.0,
        overshoot=max(first.overshoot, second.overshoot),
        steady_error=(first.steady_error + second.steady_error) / 2.0,
        settling_s=max(
            x for x in (first.settling_s, second.settling_s) if x is not None
        ) if first.settling_s is not None and second.settling_s is not None else None,
        saturation_fraction=max(
            first.saturation_fraction, second.saturation_fraction
        ),
        resolved_transient=first.resolved_transient and second.resolved_transient,
        score=(first.score + second.score) / 2.0,
    )


def score_tracking(
    samples: list[Sample],
    *,
    angle: bool,
    output_limit: float,
    max_interval_s: float = 1.0,
    min_samples: int = 12,
) -> ResponseScore:
    """Score a bounded moving-reference trajectory, including heading wrap."""
    if min_samples < 4:
        raise ValueError("minimum sample count must be at least 4")
    if len(samples) < min_samples:
        raise ValueError(f"need at least {min_samples} trajectory samples")
    if output_limit <= 0 or not math.isfinite(output_limit):
        raise ValueError("output limit must be positive")
    if max_interval_s <= 0 or not math.isfinite(max_interval_s):
        raise ValueError("maximum sample interval must be positive")
    if any(not all(math.isfinite(x) for x in asdict(s).values()) for s in samples):
        raise ValueError("nonfinite trajectory sample")
    intervals = [b.time_s - a.time_s for a, b in zip(samples, samples[1:])]
    if any(dt <= 0 or dt > max_interval_s for dt in intervals):
        raise ValueError("duplicate, reordered or stale trajectory samples")
    error = [
        wrap_deg(s.setpoint - s.process) if angle else s.setpoint - s.process
        for s in samples
    ]
    duration = samples[-1].time_s - samples[0].time_s
    scale = max(1.0, median(abs(s.setpoint) for s in samples))
    iae = sum(
        0.5 * (abs(a) + abs(b)) * (v.time_s - u.time_s)
        for a, b, u, v in zip(error, error[1:], samples, samples[1:])
    )
    saturation = mean(abs(s.output) >= output_limit * 0.98 for s in samples)
    if any(abs(s.output) > output_limit + 0.001 for s in samples):
        raise ValueError("output exceeded locked limit")
    if saturation > 0.25:
        raise ValueError("sustained saturation; reduce trajectory command")
    steady = mean(abs(x) for x in error[-max(5, len(error) // 5):]) / scale
    normalized = iae / max(duration * scale, 1e-6)
    score = normalized + steady + saturation
    return ResponseScore(
        normalized, 0.0, steady, None, saturation,
        max(abs(x) for x in error) < 0.5 * scale, score,
    )


def score_step(samples: list[Sample], *, angle: bool, output_limit: float) -> ResponseScore:
    if len(samples) < 12:
        raise ValueError("need pre-step and at least 12 measured samples")
    if not math.isfinite(output_limit) or output_limit <= 0:
        raise ValueError("output limit must be positive")
    if any(not all(math.isfinite(x) for x in asdict(s).values()) for s in samples):
        raise ValueError("nonfinite sample")
    intervals = [b.time_s-a.time_s for a, b in zip(samples, samples[1:])]
    if any(dt <= 0 or dt > 1.0 for dt in intervals):
        raise ValueError("duplicate, reordered or stale samples")
    delta = wrap_deg if angle else lambda x: x
    baseline = samples[0].setpoint
    index = next((i for i, s in enumerate(samples)
                  if abs(delta(s.setpoint-baseline)) > 1e-4), None)
    if index is None or index < 3 or len(samples)-index < 8:
        raise ValueError("need at least 3 pre-step and 8 post-step samples")
    target = samples[index].setpoint
    if any(abs(delta(s.setpoint-target)) > 0.001 for s in samples[index:]):
        raise ValueError("mixed setpoint steps")
    step = delta(target-baseline)
    initial = median(s.process for s in samples[index-3:index])
    if angle:
        initial = samples[index-1].process + median(
            wrap_deg(s.process-samples[index-1].process) for s in samples[index-3:index]
        )
    tolerance = max(abs(step)*0.05, 0.5 if angle else 1.0)
    if abs(delta(initial-baseline)) > tolerance:
        raise ValueError("baseline has not settled; do not tune from this round")
    post = samples[index:]
    errors = [delta(target-s.process) for s in post]
    progress = [delta(s.process-initial) / step for s in post]
    if max(progress) < 0.1:
        raise ValueError("no measurable response or reversed feedback sign")
    if min(progress) < -0.3:
        raise ValueError("feedback moved opposite to the requested step")
    duration = post[-1].time_s-post[0].time_s
    iae = sum(0.5*(abs(a)+abs(b))*(v.time_s-u.time_s)
              for a,b,u,v in zip(errors, errors[1:], post, post[1:]))
    overshoot = max(0.0, max(progress)-1)
    steady_errors = errors[-max(3, len(errors)//5):]
    steady = math.sqrt(mean(value * value for value in steady_errors)) / abs(step)
    saturated = mean(abs(s.output) >= output_limit*0.98 for s in post)
    if any(abs(s.output) > output_limit+0.001 for s in post):
        raise ValueError("output exceeded locked limit")
    if saturated > 0.25:
        raise ValueError("sustained saturation; reduce the experiment step")
    window = max(3, round(0.2 / median(intervals)))
    window_rms = [
        math.sqrt(mean(value * value for value in errors[i:i + window]))
        for i in range(len(errors) - window + 1)
    ]
    settling_index = next((i for i in range(len(window_rms))
                           if all(value <= tolerance for value in window_rms[i:])), None)
    settling = (post[settling_index].time_s - post[0].time_s
                if settling_index is not None else None)
    # At 5 Hz a rise completed in one sample cannot justify more aggressive gains.
    transient_samples = sum(0.1 < p < 0.9 for p in progress)
    normalized = iae / (duration*abs(step))
    response_time = 1.0 if settling is None else settling / max(duration, 1e-6)
    score = (STEP_SCORE_WEIGHTS["normalized_iae"]*normalized +
             STEP_SCORE_WEIGHTS["overshoot"]*overshoot +
             STEP_SCORE_WEIGHTS["steady_error_rms"]*steady +
             STEP_SCORE_WEIGHTS["normalized_settling"]*response_time +
             STEP_SCORE_WEIGHTS["saturation_fraction"]*saturated)
    return ResponseScore(normalized, overshoot, steady, settling, saturated,
                         transient_samples >= 2, score)


def candidate(gains: tuple[float, float], result: ResponseScore,
              loop: str) -> tuple[float, float]:
    """Bounded heuristic proposals evaluated by the next measured round."""
    if loop not in ("angular_velocity", "direction"):
        raise ValueError("this adapter supports only VGT6 PI rate and PD heading")
    p, second = gains
    if result.overshoot > 0.10:
        if loop == "angular_velocity":
            second *= 0.85
        else:
            p *= 0.9
            second *= 1.2
    elif result.steady_error > 0.08 and loop == "angular_velocity":
        second *= 1.2
    elif result.resolved_transient and result.normalized_iae > 0.08:
        p *= 1.15
    # A zero coefficient is deliberately not invented by this heuristic.
    return tuple(round(max(0, min(100, x)), 4) for x in (p, second))
