"""Bind the pure search campaign to the existing stopped HLOG session."""
import csv
import json
from dataclasses import asdict

from bounded_search import guarded_step
from closed_loop_tuning import STEP_SCORE_WEIGHTS, wrap_deg
from hlog_experiment import score_logged
from tuning_campaign import run_campaign, validate_result
from loop_profiles import get_profile


async def execute_campaign(args, session, plan, inner, heading_reference, save_raw):
    applied = plan.initial
    experiment_index = 0
    active_index = None
    logs, writes = [], []
    latest = {}
    angle = args.loop == "direction"
    convert = wrap_deg if angle else lambda x: x
    offset = heading_reference if angle else 0.0
    metadata = {"loop": args.loop, "experiment": getattr(args, "experiment", "step"),
                "firmware": session.firmware_version, "capabilities": session.expected_caps,
                "form": "parallel", "period_s": 0.02,
                "derivative": "measurement" if angle else "none",
                "scoring_source": "downloaded_HLOG", "log_period_s": 0.02,
                "timestamp_unit": "us" if session.require_hlog_us else "ms",
                "limits": {"wheel_rpm": 160, "rotation_rpm": 60, "rate_dps": 80},
                "training": {"baseline": args.baseline, "target": args.target,
                             "seconds": args.seconds, "baseline_seconds": args.baseline_seconds},
                "inner_gains": inner, "wheel_gains": session.wheel_baseline,
                "ram_only": True}
    metadata["profile"] = get_profile(args.loop)
    metadata["objective"] = {"step_weights": STEP_SCORE_WEIGHTS,
                             "repeat_aggregation": "mean + 0.5 * sample_stdev",
                             "spin_aggregation": "mean of step scores; worst overshoot/saturation",
                             "acceptance": "independent holdout improvement plus per-case metric guards"}

    def checkpoint(report):
        nonlocal latest
        latest = {**metadata, **report, "logs": logs, "writes": writes,
                  "applied_gains": applied}
        temporary = args.output / "session.json.tmp"
        temporary.write_text(json.dumps(latest, indent=2, allow_nan=False), encoding="utf-8")
        temporary.replace(args.output / "session.json")

    async def unchanged(expected):
        await session.health()
        if await session.get_gains(args.loop) != tuple(expected):
            raise RuntimeError("external PID change; refusing campaign write or score")
        if angle and await session.get_gains("angular_velocity") != inner:
            raise RuntimeError("inner PID changed during outer-loop campaign")

    async def apply(expected, target):
        nonlocal applied
        await session.stop_and_confirm()
        await unchanged(expected)
        actual = tuple(expected)
        for _ in range(256):
            if actual == tuple(target):
                applied = actual
                return
            step = guarded_step(actual, target)
            if step == actual:
                raise RuntimeError("cannot restore gains within locked per-write guard")
            await unchanged(actual)
            await session.write(args.loop, step)
            writes.append({"old": actual, "new": step, "readback": step})
            actual = applied = step
        raise RuntimeError("bounded restoration exceeded write budget")

    async def evaluate(gains, stage, target, repeat):
        nonlocal experiment_index, active_index
        await unchanged(gains)
        active_index = experiment_index
        experiment_index += 1
        phases = [(convert(offset+args.baseline), args.baseline_seconds),
                  (convert(offset+(args.target if target is None else target)), args.seconds)]
        if target is None and getattr(args, "experiment", "step") == "spin":
            phases[1] = (abs(args.target), args.seconds)
            phases.append((-abs(args.target), args.seconds))
        log = {"index": active_index+1, "stage": stage, "target": target,
               "repeat": repeat+1, "gains": gains, "phases": phases,
               "raw": str(args.output / f"round-{active_index+1:02d}-raw.json")}
        logs.append(log)
        try:
            records = await session.collect_logged(args.loop, phases)
            save_raw(args, active_index, session, gains, records, phases=phases)
            measured, result = score_logged(records, args.loop, phases)
            await unchanged(gains)
            path = args.output / f"round-{active_index+1:02d}.csv"
            with path.open("w", newline="", encoding="utf-8") as stream:
                writer = csv.writer(stream)
                writer.writerow(("time_s", "setpoint", "process", "output"))
                writer.writerows((x.time_s, x.setpoint, x.process, x.output) for x in measured)
            path = args.output / f"round-{active_index+1:02d}-hlog.csv"
            with path.open("w", newline="", encoding="utf-8") as stream:
                writer = csv.DictWriter(stream, fieldnames=list(records[0]))
                writer.writeheader()
                writer.writerows(records)
            log.update(metrics=asdict(result), data_quality="passed")
            validate_result(result, plan.max_overshoot)
            log["quality"] = "passed"
            return result
        except BaseException as exc:
            log.update(quality="failed", error=str(exc) or type(exc).__name__)
            save_raw(args, active_index, session, gains, session.download["records"],
                     phases=phases, error=log["error"])
            raise

    checkpoint({"plan": asdict(plan), "status": "running", "history": [],
                "best_gains": None, "selection": "pending_holdout"})
    try:
        return await run_campaign(plan, evaluate, apply, checkpoint)
    except BaseException as exc:
        failure = {**latest, "status": "failed", "error": str(exc) or type(exc).__name__,
                   "restored_readback": None}
        try:
            await session.stop_and_confirm()
            # A quality failure does not authorize writing across a link/health
            # fault or an external gain change. Restore only a validated baseline.
            if latest.get("baseline_validated"):
                await apply(applied, plan.initial)
                failure["restored_readback"] = plan.initial
        except BaseException as restore_error:
            failure["restoration_error"] = str(restore_error) or type(restore_error).__name__
        checkpoint(failure)
        raise
