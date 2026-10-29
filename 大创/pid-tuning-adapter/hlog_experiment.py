"""Fail-closed experiment planning and scoring for R17 downloaded RAM logs."""
from statistics import mean
import math

from closed_loop_tuning import score_bidirectional, score_step, wrap_deg
from pid_tuning import Sample
from pid_ws_logger import hlog_samples

MAX_RECORDING_S = 18.0  # 1024 * 20 ms, with command/stop latency margin.


def recording_budget(args):
    experiment = getattr(args, "experiment", "step")
    if experiment == "ground_speed":
        return None  # Observation only, never an automatic wheel gain writer.
    if args.loop not in ("angular_velocity", "direction"):
        raise ValueError("high-rate attitude tuning supports only rate PI / heading PD")
    if (not all(math.isfinite(x) for x in
                (args.seconds, args.baseline_seconds, args.baseline, args.target)) or
            not 3 <= args.seconds or not 3 <= args.baseline_seconds or
            not 1 <= args.rounds <= 20):
        raise ValueError("invalid durations, targets or round count")
    bound = 80 if args.loop == "angular_velocity" else 180
    step = (wrap_deg(args.target - args.baseline) if args.loop == "direction"
            else args.target - args.baseline)
    if max(abs(args.target), abs(args.baseline)) > bound or not 5 <= abs(step) <= 60:
        raise ValueError("target exceeds locked experiment bounds")
    if experiment == "heading_round_trip":
        raise ValueError("automatic moving-heading tuning is disabled; use step or direction_identify")
    if experiment == "spin" and (args.loop != "angular_velocity" or args.baseline != 0):
        raise ValueError("spin requires angular_velocity and zero baseline")
    if experiment == "direction_identify" and args.loop != "direction":
        raise ValueError("direction_identify requires direction")
    duration = (args.seconds + 1.0 if experiment == "direction_identify" else
                args.baseline_seconds + args.seconds * (2 if experiment == "spin" else 1))
    if not 0 < duration <= MAX_RECORDING_S:
        raise ValueError(f"HLOG experiment needs {duration:g}s; maximum is {MAX_RECORDING_S:g}s with margin")
    return duration


def score_logged(records, loop, phases):
    """Use actual MCU timestamps/reference edges, never host arrival timing."""
    samples = [Sample(*row) for row in hlog_samples(records, loop, max_interval_ms=25)]
    angle = loop == "direction"
    delta = wrap_deg if angle else lambda x: x
    blocks = []
    for sample in samples:
        if not blocks or abs(delta(sample.setpoint - blocks[-1][0].setpoint)) > 0.001:
            blocks.append([])
        blocks[-1].append(sample)
    if len(blocks) != len(phases):
        raise ValueError("HLOG reference edges do not match the locked experiment")
    for block, (target, seconds) in zip(blocks, phases):
        if abs(delta(block[0].setpoint - target)) > 0.002:
            raise ValueError("HLOG target does not match command timeline")
        if block[-1].time_s - block[0].time_s < seconds - 0.4:
            raise ValueError("HLOG missing baseline, transient or final observation coverage")
    if any(x.get("forward_rpm") or x.get("left_rpm") for x in records):
        raise ValueError("unexpected translation in a stationary attitude step")
    if any(not math.isfinite(x["actual_rate_dps"]) or abs(x["actual_rate_dps"]) > 120
           for x in records):
        raise ValueError("logged yaw rate outside session guard")
    if mean(bool(x["saturated"]) for x in records) > 0.25:
        raise ValueError("sustained controller saturation")
    scores = []
    for before, after in zip(blocks, blocks[1:]):
        tail = [x for x in before if x.time_s >= before[-1].time_s - 1.0]
        step = delta(after[0].setpoint - before[0].setpoint)
        tolerance = max(abs(step) * 0.05, 0.5 if angle else 1.0)
        baseline_errors = [delta(x.process - before[0].setpoint) for x in tail]
        baseline_rms = math.sqrt(mean(value * value for value in baseline_errors))
        if baseline_rms > tolerance:
            raise ValueError("pre-step baseline has not settled for a full second")
        initial = before[-1].process
        transient = [x for x in after if 0.1 < delta(x.process - initial) / step < 0.9]
        if len(transient) < 10:
            raise ValueError("fewer than 10 independent transient samples; no gain decision")
        tail_start = after[-1].time_s - 1.0
        midpoint = after[-1].time_s - 0.5
        first_half = [x.process for x in after
                      if tail_start <= x.time_s < midpoint]
        second_half = [x.process for x in after if x.time_s >= midpoint]
        if min(len(first_half), len(second_half)) < 10:
            raise ValueError("insufficient final-window coverage")
        if abs(mean(first_half) - mean(second_half)) > tolerance:
            raise ValueError("final response is still drifting; extend acquisition, do not tune")
        final_errors = [delta(after[-1].setpoint - x.process) for x in after
                        if x.time_s >= tail_start]
        final_rms = math.sqrt(mean(value * value for value in final_errors))
        if final_rms > tolerance:
            raise ValueError("final tracking RMS exceeds the locked tolerance")
        scores.append(tail + after)
    limit = 80 if angle else 60
    result = (score_bidirectional(*scores, angle=angle, output_limit=limit)
              if len(scores) == 2 else score_step(scores[0], angle=angle, output_limit=limit))
    return samples, result
