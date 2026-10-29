import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from campaign_runner import execute_campaign
from closed_loop_tuning import ResponseScore
from pid_tuning import Sample
from tuning_campaign import CampaignPlan


class CampaignBindingTests(unittest.IsolatedAsyncioTestCase):
    async def failed_candidate(self, link_fault=False, overshoot=False):
        initial = (1.0, 2.0)
        plan = CampaignPlan("angular_velocity", initial, ((0.6, 1.4), (1.2, 2.8)),
                            (-10, 10), method="heuristic", rounds=2)
        gains, calls, stopped, writes = initial, 0, 0, []
        fault = False
        async def health():
            if fault:
                raise RuntimeError("link unhealthy")
        async def get_gains(loop):
            return gains
        async def stop():
            nonlocal stopped
            stopped += 1
        async def write(loop, target):
            nonlocal gains
            writes.append(target)
            gains = target
        async def collect(loop, phases):
            nonlocal calls, fault
            calls += 1
            if calls == 7 and not overshoot:
                fault = link_fault
                raise ValueError("log gap")
            return [{"sequence": 0}]
        session = SimpleNamespace(firmware_version="test", expected_caps="test",
                                  require_hlog_us=False, wheel_baseline=(1,)*12,
                                  health=health, get_gains=get_gains, stop_and_confirm=stop,
                                  write=write, collect_logged=collect,
                                  download={"records": []})
        saved = []
        with tempfile.TemporaryDirectory() as directory:
            args = SimpleNamespace(loop="angular_velocity", baseline=0, target=15,
                                   seconds=6, baseline_seconds=4, output=Path(directory))
            def save(*pos, **kwargs):
                saved.append((pos[1], kwargs))
            def score(*args):
                return [Sample(0, 0, 0, 0)], ResponseScore(
                    0.5, 0.4 if calls == 7 and overshoot else 0, 0, 1, 0, True, 1)
            with patch("campaign_runner.score_logged", side_effect=score):
                with self.assertRaisesRegex(ValueError, "performance constraints" if overshoot else "log gap"):
                    await execute_campaign(args, session, plan, initial, None, save)
            report = json.loads((Path(directory)/"session.json").read_text())
        self.assertGreater(stopped, 0)
        self.assertIn("performance constraints" if overshoot else "log gap", saved[-1][1]["error"])
        self.assertEqual(report["status"], "failed")
        self.assertEqual(report["logs"][-1]["quality"], "failed")
        return gains, report, writes

    async def test_quality_failure_stops_and_restores_only_validated_baseline(self):
        gains, report, writes = await self.failed_candidate()
        self.assertEqual(gains, (1, 2))
        self.assertEqual(report["restored_readback"], [1, 2])
        self.assertEqual(len(writes), 2)

    async def test_health_failure_stops_and_reports_restoration_pending(self):
        gains, report, writes = await self.failed_candidate(True)
        self.assertNotEqual(gains, (1, 2))
        self.assertIsNone(report["restored_readback"])
        self.assertIn("link unhealthy", report["restoration_error"])
        self.assertEqual(len(writes), 1)

    async def test_performance_failure_is_recorded_as_failed_raw_and_restored(self):
        gains, report, _ = await self.failed_candidate(overshoot=True)
        self.assertEqual(gains, (1, 2))
        self.assertEqual(report["logs"][-1]["data_quality"], "passed")
        self.assertGreater(report["logs"][-1]["metrics"]["overshoot"], 0.3)


if __name__ == "__main__":
    unittest.main()
