import unittest
from types import SimpleNamespace

from closed_loop_tuning import ResponseScore
from tuning_campaign import CampaignPlan, accepts_holdout, make_plan, run_campaign, summarize


def result(score, overshoot=0.0):
    return ResponseScore(score/2, overshoot, 0.0, 1.0, 0.0, True, score)


class CampaignTests(unittest.IsolatedAsyncioTestCase):
    async def campaign(self, holdout_good=True, fail=False):
        initial = (1.0, 1.0)
        plan = CampaignPlan("angular_velocity", initial, ((0.6, 1.4), (0.6, 1.4)),
                            (-10, 10), rounds=3, repeats=2)
        writes, measured, reports = [], [], []
        async def evaluate(gains, stage, target, repeat):
            measured.append((gains, stage, target, repeat))
            if fail and gains != initial:
                raise ValueError("invalid HLOG")
            good = gains != initial and (target is None or holdout_good)
            return result((0.5 if good else 1.0)+repeat*0.001)
        async def apply(old, new):
            writes.append((old, new))
        report = await run_campaign(plan, evaluate, apply, reports.append)
        return report, writes, measured

    async def test_finalist_needs_unseen_repeated_holdout_and_restoration(self):
        report, writes, measured = await self.campaign()
        self.assertEqual(report["selection"], "holdout_improved")
        self.assertEqual(report["optimizer_observations"], 3)
        self.assertEqual(len([x for x in measured if x[1] == "candidate_holdout"]), 4)
        self.assertEqual(writes[-1][1], tuple(report["best_gains"]))
        self.assertTrue(all(x[2] is None for x in measured if x[1] == "train"))

    async def test_better_training_cannot_replace_baseline_without_holdout_improvement(self):
        report, writes, _ = await self.campaign(False)
        self.assertEqual(report["best_gains"], (1, 1))
        self.assertEqual(report["selection"], "holdout_rejected_restore_baseline")
        self.assertEqual(writes[-1][1], (1, 1))

    async def test_bad_trial_aborts_without_ranking_or_final_validation(self):
        with self.assertRaisesRegex(ValueError, "invalid HLOG"):
            await self.campaign(fail=True)

    def test_noise_and_worst_case_prevent_false_acceptance(self):
        baseline = [summarize([result(1), result(1.01)])]*2
        noisy = [summarize([result(0.1), result(1.8)])]*2
        self.assertFalse(accepts_holdout(noisy, baseline, 0.01))
        worsening = [summarize([result(0.4, 0.2), result(0.4, 0.2)])]*2
        self.assertFalse(accepts_holdout(worsening, baseline, 0.01))

    def test_plan_rejects_train_holdout_overlap_and_invalid_bounds(self):
        args = SimpleNamespace(loop="angular_velocity", target=15, baseline=0, rounds=3,
                               validation_targets=[-15, 15])
        with self.assertRaisesRegex(ValueError, "different"):
            make_plan(args, (0.15, 2.5))
        args.validation_targets = [-10, 10]
        args.kp_bounds = [1, 2]
        with self.assertRaisesRegex(ValueError, "initial gains"):
            make_plan(args, (0.15, 2.5))


if __name__ == "__main__":
    unittest.main()
