"""Score measured closed-loop steps; never mistake reference for actuator input."""
from __future__ import annotations
import math
from dataclasses import asdict, dataclass
from statistics import mean, median
from pid_tuning import Sample


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
    steady = abs(mean(errors[-max(3, len(errors)//5):])) / abs(step)
    saturated = mean(abs(s.output) >= output_limit*0.98 for s in post)
    if any(abs(s.output) > output_limit+0.001 for s in post):
        raise ValueError("output exceeded locked limit")
    if saturated > 0.25:
        raise ValueError("sustained saturation; reduce the experiment step")
    settling = next((post[i].time_s-post[0].time_s for i in range(len(post)-2)
                     if all(abs(e) <= tolerance for e in errors[i:])), None)
    # At 5 Hz a rise completed in one sample cannot justify more aggressive gains.
    transient_samples = sum(0.1 < p < 0.9 for p in progress)
    normalized = iae / (duration*abs(step))
    score = normalized + 2*overshoot + steady + saturated
    return ResponseScore(normalized, overshoot, steady, settling, saturated,
                         transient_samples >= 2, score)


def candidate(gains: tuple[float, float], result: ResponseScore,
              loop: str) -> tuple[float, float]:
    """Bounded heuristic proposals evaluated by the next measured round."""
    if loop not in ("angular_velocity", "direction"):
        raise ValueError("this adapter supports only VGT6 PI rate and PD heading")
    p, second = gains
    if result.overshoot > 0.10:
        p *= 0.9
        second *= 0.85 if loop == "angular_velocity" else 1.2
    elif result.steady_error > 0.08 and loop == "angular_velocity":
        second *= 1.2
    elif result.resolved_transient and result.normalized_iae > 0.08:
        p *= 1.15
    # A zero coefficient is deliberately not invented by this heuristic.
    return tuple(round(max(0, min(100, x)), 4) for x in (p, second))
