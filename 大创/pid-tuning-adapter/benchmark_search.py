#!/usr/bin/env python3
"""Offline PI/PD comparisons through the real HLOG scorer, never hardware.

These deliberately simple plants are reproducible regression scenarios, not
identified BM50 models. Synthetic logs are always labelled simulation_only.
"""
from __future__ import annotations

import argparse
import asyncio
import json
import math
import random
from dataclasses import asdict
from pathlib import Path
from statistics import mean

from hlog_experiment import score_logged
from tuning_campaign import CampaignPlan, run_campaign


def simulate(loop, gains, target, *, seed=0, noise=0.02, load=1.0):
    rng = random.Random(seed)
    dt = 0.02
    value = rate = integral = 0.0
    rows = []
    delay = [0.0]*3
    for i in range(500):
        reference = 0.0 if i < 200 else target
        feedback = value+rng.gauss(0, noise)
        error = reference-feedback
        if loop == "angular_velocity":
            proposed = gains[0]*error+integral
            output = max(-60.0, min(60.0, proposed))
            if abs(proposed) < 60 or proposed*error < 0:
                integral += gains[1]*error*dt
            delay.append(output)
            value += dt*(0.9*load*delay.pop(0)-value)/0.65
            actual_rate = feedback
        else:
            proposed = gains[0]*error-gains[1]*rate
            output = max(-80.0, min(80.0, proposed))
            delay.append(output)
            rate += dt*(load*delay.pop(0)-rate)/0.3
            value += dt*rate
            actual_rate = rate
        angle = loop == "direction"
        rows.append({"sequence": i, "tick": 1000+20*i,
                     "tick_us": 1000000+20000*i, "mode": 2 if angle else 1,
                     "target_heading_deg": reference if angle else 0.0,
                     "actual_heading_deg": feedback if angle else 0.0,
                     "heading_error_deg": error if angle else 0.0,
                     "target_rate_dps": output if angle else reference,
                     "actual_rate_dps": actual_rate,
                     "heading_output_dps": output if angle else 0.0,
                     "rate_output_rpm": 0.0 if angle else output,
                     "saturated": abs(proposed) >= (80 if angle else 60),
                     "forward_rpm": 0, "left_rpm": 0})
    return score_logged(rows, loop, [(0, 4), (target, 6)])[1]


async def compare(loop, method, seed, rounds, noise):
    initial = (0.15, 2.5) if loop == "angular_velocity" else (5.0, 1.25)
    plan = CampaignPlan(loop, initial,
                        tuple((round(x*0.6, 4), round(x*1.4, 4)) for x in initial),
                        (-10.0, 10.0), method, rounds, 2, seed)
    report = {}
    trials, failures, bounds_violations = 0, 0, 0
    last_gains = initial

    def checkpoint(value):
        nonlocal report
        # Detach from the running object for an honest failure snapshot.
        report = json.loads(json.dumps(value))

    async def apply(current, target):
        nonlocal last_gains, bounds_violations
        if any(not lo <= gain <= hi for gain, (lo, hi) in zip(target, plan.bounds)):
            bounds_violations += 1
            raise ValueError("candidate outside global bounds")
        last_gains = target

    async def evaluate(gains, stage, target, repeat):
        nonlocal trials, failures
        trials += 1
        held_out = target is not None
        # Common random numbers across methods; a second load is unseen by search.
        experiment_seed = seed*10000 + repeat + (1000 if held_out else 0) + (100 if (target or 15) < 0 else 0)
        try:
            result = simulate(loop, gains, 15 if target is None else target,
                              seed=experiment_seed, noise=noise,
                              load=0.95 if held_out else 1.0)
            if result.overshoot > plan.max_overshoot:
                raise ValueError("overshoot hard constraint")
            return result
        except ValueError:
            failures += 1
            raise

    error = None
    try:
        await run_campaign(plan, evaluate, apply, checkpoint)
    except ValueError as exc:
        error = str(exc)
    training = report.get("training_trials", [])
    incumbent = float("inf")
    convergence = []
    for row in training:
        incumbent = min(incumbent, row["objective"])
        convergence.append(incumbent)
    return {"loop": loop, "method": method, "seed": seed, "noise": noise,
            "evidence": "simulation_only", "plan": asdict(plan),
            "trials": trials, "failed_trials": failures,
            "bounds_violations": bounds_violations,
            "status": report.get("status", "failed") if error is None else "failed",
            "error": error, "selection": report.get("selection"),
            "initial_score": training[0]["objective"] if training else None,
            "best_training_curve": convergence, "best_score": report.get("best_score"),
            "best_gains": report.get("best_gains"), "last_simulated_gains": last_gains,
            "campaign": report}


async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seeds", type=int, default=3)
    parser.add_argument("--rounds", type=int, default=10)
    args = parser.parse_args()
    if not 1 <= args.seeds <= 20 or not 2 <= args.rounds <= 20:
        parser.error("seeds must be 1..20 and rounds 2..20")
    args.output.mkdir(parents=True, exist_ok=False)
    results = []
    for loop in ("angular_velocity", "direction"):
        for noise in (0.0, 0.05):
            for method in ("heuristic", "bo", "de"):
                for seed in range(args.seeds):
                    results.append(await compare(loop, method, seed, args.rounds, noise))
    (args.output / "comparison.json").write_text(
        json.dumps({"evidence": "simulation_only", "results": results}, indent=2, allow_nan=False),
        encoding="utf-8")
    lines = ["# Offline search comparison", "", "Evidence: simulation_only. No physical vehicle was accessed.",
             "These fixed synthetic plants do not establish BM50 performance or a universally superior searcher.",
             "Held-out cases use both directions, another amplitude and a different plant gain.", "",
             "| Loop | Noise SD | Method | Completed | Holdout accepted | Mean trials | Failed trials | Bound violations | Mean retained score |",
             "|---|---:|---|---:|---:|---:|---:|---:|---:|"]
    for loop in ("angular_velocity", "direction"):
        for noise in (0.0, 0.05):
            for method in ("heuristic", "bo", "de"):
                subset = [x for x in results if (x["loop"], x["noise"], x["method"]) == (loop, noise, method)]
                scored = [x["best_score"] for x in subset if x["status"] == "complete"]
                score = f"{mean(scored):.5f}" if scored else "n/a"
                lines.append(f"| {loop} | {noise} | {method} | {sum(x['status']=='complete' for x in subset)}/{len(subset)} | "
                             f"{sum(x['selection']=='holdout_improved' and x['status']=='complete' for x in subset)} | "
                             f"{mean(x['trials'] for x in subset):.1f} | {sum(x['failed_trials'] for x in subset)} | "
                             f"{sum(x['bounds_violations'] for x in subset)} | {score} |")
    lines.extend(["", "The JSON retains seeds, bounds, failure reasons, per-trial metrics, dispersion and best-score convergence curves.",
                  "Failures abort a campaign and never enter score ranking. Completed counts must be read alongside retained scores."])
    (args.output / "comparison.md").write_text("\n".join(lines)+"\n", encoding="utf-8")
    print(args.output / "comparison.md")


if __name__ == "__main__":
    asyncio.run(main())
