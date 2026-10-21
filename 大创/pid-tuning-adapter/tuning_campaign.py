"""Repeat -> bounded search -> independent holdout -> verified restoration.

The evaluator owns log provenance/quality and must raise on a failed trial.
Holdout scores are deliberately never passed to the optimizer.
"""
from __future__ import annotations

import math
from dataclasses import asdict, dataclass
from statistics import mean, stdev

from bounded_search import BoundedSearch
from closed_loop_tuning import candidate, wrap_deg
from loop_profiles import get_profile


@dataclass(frozen=True)
class CampaignPlan:
    loop: str
    initial: tuple[float, float]
    bounds: tuple[tuple[float, float], tuple[float, float]]
    validation_targets: tuple[float, ...]
    method: str = "bo"
    rounds: int = 5  # Includes the incumbent's repeated training trial.
    repeats: int = 2
    seed: int = 0
    min_improvement: float = 0.01
    max_overshoot: float = 0.30


def make_plan(args, initial):
    get_profile(args.loop)
    delta = wrap_deg(args.target-args.baseline) if args.loop == "direction" else args.target-args.baseline
    spin = getattr(args, "experiment", "step") == "spin"
    targets = getattr(args, "validation_targets", None)
    if targets is None:
        other = abs(delta)*0.5 if abs(delta) >= 10 else abs(delta)*2
        targets = (-other, other) if spin else (args.baseline-delta, args.baseline+math.copysign(other, delta))
        if args.loop == "direction":
            targets = tuple(wrap_deg(x) for x in targets)
    targets = tuple(targets)
    bound = 80 if args.loop == "angular_velocity" else 180
    training_targets = (abs(args.target), -abs(args.target)) if spin else (args.target,)
    difference = wrap_deg if args.loop == "direction" else lambda x: x
    if len(targets) < 2 or len(set(targets)) != len(targets):
        raise ValueError("declare at least two distinct holdout targets")
    if any(not math.isfinite(x) or abs(x) > bound or
           not 5 <= abs(difference(x-args.baseline)) <= 60 or
           any(abs(difference(x-t)) < 0.001 for t in training_targets) for x in targets):
        raise ValueError("holdout must use different, bounded steps (5..60 loop units)")
    steps = [difference(t-args.baseline) for t in (*targets, *training_targets)]
    if not min(steps) < 0 < max(steps):
        raise ValueError("campaign must validate both response directions")
    bounds = tuple(tuple(getattr(args, name, None) or
                         (round(value*0.6, 4), min(100.0, round(value*1.4, 4))))
                   for name, value in zip(("kp_bounds", "second_bounds"), initial))
    plan = CampaignPlan(args.loop, tuple(initial), bounds, targets,
                        getattr(args, "search", "bo"), args.rounds,
                        getattr(args, "repeats", 2), getattr(args, "seed", 0),
                        getattr(args, "min_improvement", 0.01),
                        getattr(args, "max_overshoot", 0.30))
    if not 2 <= plan.repeats <= 10 or not 1 <= plan.rounds <= 20:
        raise ValueError("repeats must be 2..10 and rounds 1..20")
    if (not math.isfinite(plan.min_improvement) or plan.min_improvement < 0 or
            not math.isfinite(plan.max_overshoot) or not 0 < plan.max_overshoot <= 1):
        raise ValueError("invalid improvement/overshoot threshold")
    search = BoundedSearch(plan.bounds, method=plan.method, seed=plan.seed)
    if not search.contains(initial):
        raise ValueError("initial gains must lie inside the declared bounds")
    return plan


def summarize(results):
    scores = [row.score for row in results]
    sd = stdev(scores) if len(scores) > 1 else 0.0
    return {"mean": mean(scores), "stdev": sd, "worst": max(scores),
            "variance_of_mean": sd*sd/len(scores), "count": len(scores),
            "objective": mean(scores)+0.5*sd,
            "metrics": [asdict(row) for row in results]}


def validate_result(result, max_overshoot):
    if (not all(math.isfinite(x) and x >= 0 for x in
                (result.score, result.overshoot, result.steady_error,
                 result.normalized_iae, result.saturation_fraction)) or
            result.overshoot > max_overshoot or
            result.saturation_fraction > 0.25 or not result.resolved_transient):
        raise ValueError("trial failed locked performance constraints")


def improves(challenger, incumbent, minimum):
    uncertainty = 1.96*math.sqrt(challenger["variance_of_mean"]+incumbent["variance_of_mean"])
    return incumbent["mean"]-challenger["mean"] > max(minimum, uncertainty)


def accepts_holdout(challenger, incumbent, minimum):
    """Require improvement overall and no material regression in any case."""
    if not challenger or len(challenger) != len(incumbent):
        return False
    for new, old in zip(challenger, incumbent):
        if new["mean"] > old["mean"]+0.01 or new["worst"] > old["worst"]+0.01:
            return False
        for metric in ("overshoot", "steady_error", "saturation_fraction"):
            if max(x[metric] for x in new["metrics"]) > max(x[metric] for x in old["metrics"])+0.01:
                return False
        new_settle = [x["settling_s"] for x in new["metrics"]]
        old_settle = [x["settling_s"] for x in old["metrics"]]
        if all(x is not None for x in old_settle) and (
                any(x is None for x in new_settle) or max(new_settle) > max(old_settle)+0.1):
            return False
    new_all = summarize_scores(challenger)
    old_all = summarize_scores(incumbent)
    return improves(new_all, old_all, minimum) and new_all["worst"] <= old_all["worst"]+0.01


def summarize_scores(cases):
    # Equal repeats/case are locked in the plan; summarize the individual scores.
    from closed_loop_tuning import ResponseScore
    return summarize([ResponseScore(**row) for case in cases for row in case["metrics"]])


async def run_campaign(plan, evaluate, apply, checkpoint):
    """evaluate(gains, stage, target, repeat) -> a validated ResponseScore.

    apply() must STOP, guard every step, ACK and read back, including rollback.
    checkpoint() persists each completed experiment and selection rationale.
    An exception aborts; the hardware caller handles STOP and safe restoration.
    """
    search = BoundedSearch(plan.bounds, method=plan.method, seed=plan.seed)
    report = {"plan": asdict(plan), "status": "running", "history": [],
              "best_gains": None, "best_score": None, "selection": "pending_holdout",
              "evidence": "validated_downloaded_WLOG" if plan.loop == "wheel_speed" else "validated_downloaded_HLOG"}
    current = plan.initial

    async def measure(gains, stage, target=None):
        results = []
        for repeat in range(plan.repeats):
            result = await evaluate(gains, stage, target, repeat)
            validate_result(result, plan.max_overshoot)
            results.append(result)
            report["history"].append({"gains": gains, "stage": stage,
                                      "target": target, "repeat": repeat+1,
                                      "metrics": asdict(result)})
            checkpoint(report)
        return results, summarize(results)

    initial_results, initial_train = await measure(current, "train")
    search.observe(current, initial_train["objective"], variance=initial_train["variance_of_mean"])
    initial_validation = []
    for target in plan.validation_targets:
        _, summary = await measure(current, "baseline_holdout", target)
        initial_validation.append(summary)
    report.update(best_gains=plan.initial, best_score=initial_train["objective"],
                  baseline_validated=True, baseline_holdout=initial_validation,
                  training_trials=[{"gains": current, **initial_train}])
    checkpoint(report)
    finalist, finalist_train, last_result = current, initial_train, initial_results[-1]
    finalist_result = last_result
    for _ in range(1, plan.rounds):
        # Explore around the incumbent rather than letting rejected proposals
        # turn the local write neighborhood into an uncontrolled random walk.
        if current != finalist:
            await apply(current, finalist)
            current, last_result = finalist, finalist_result
        heuristic = None if plan.loop == "wheel_speed" else candidate(current, last_result, plan.loop)
        proposal = search.ask(current, heuristic=heuristic)
        if proposal is None:
            report["stop_reason"] = "no_unmeasured_reachable_candidate"
            break
        await apply(current, proposal)
        current = proposal
        results, summary = await measure(current, "train")
        search.observe(current, summary["objective"], variance=summary["variance_of_mean"])
        report["training_trials"].append({"gains": current, **summary})
        if improves(summary, finalist_train, plan.min_improvement):
            finalist, finalist_train = current, summary
            finalist_result = results[-1]
        last_result = results[-1]
        checkpoint(report)
    if finalist != plan.initial:
        await apply(current, finalist)
        current = finalist
        validation = []
        for target in plan.validation_targets:
            _, summary = await measure(current, "candidate_holdout", target)
            validation.append(summary)
        report["candidate_holdout"] = validation
        accepted = accepts_holdout(validation, initial_validation, plan.min_improvement)
        if accepted:
            report.update(best_gains=finalist, best_score=finalist_train["objective"])
        report["selection"] = "holdout_improved" if accepted else "holdout_rejected_restore_baseline"
    else:
        report["selection"] = "no_significant_training_improvement"
    await apply(current, tuple(report["best_gains"]))
    report.update(status="complete", restored_readback=report["best_gains"],
                  optimizer_observations=len(search.observations))
    checkpoint(report)
    return report
