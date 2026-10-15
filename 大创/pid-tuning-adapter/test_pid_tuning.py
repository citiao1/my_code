import csv
import tempfile
import unittest
from pathlib import Path

from pid_tuning import identify_fopdt, load_csv, tune_fopdt


class GenericTuningTests(unittest.TestCase):
    def make_step_csv(self):
        handle = tempfile.NamedTemporaryFile(mode="w", suffix=".csv", delete=False, newline="", encoding="utf-8")
        writer = csv.writer(handle)
        writer.writerow(("time_s", "setpoint", "process", "output"))
        for index in range(80):
            time_s = index * 0.1
            setpoint = 0.0 if index < 10 else 10.0
            process = 0.0 if index < 15 else 10.0 * (1.0 - pow(2.718281828, -(index - 15) / 10.0))
            writer.writerow((time_s, setpoint, process, 50.0 if index >= 10 else 0.0))
        handle.close()
        return Path(handle.name)

    def test_identifies_and_converts_all_controller_forms(self):
        path = self.make_step_csv()
        try:
            samples = load_csv(path)
            model = identify_fopdt(samples)
            self.assertGreater(model.process_gain, 0.0)
            for mode in ("P", "PI", "PD", "PID"):
                for form in ("parallel", "incremental", "position-discrete"):
                    candidate = tune_fopdt(model, mode=mode, form=form, sample_time_s=0.01)
                    self.assertGreaterEqual(candidate.kp, 0.0)
                    self.assertEqual(candidate.mode, mode)
        finally:
            path.unlink()

    def test_incremental_conversion_matches_discrete_semantics(self):
        path = self.make_step_csv()
        try:
            model = identify_fopdt(load_csv(path))
            continuous = tune_fopdt(model, mode="PI", form="parallel")
            incremental = tune_fopdt(model, mode="PI", form="incremental", sample_time_s=0.01)
            self.assertAlmostEqual(incremental.ki, continuous.ki * 0.01)
        finally:
            path.unlink()


if __name__ == "__main__":
    unittest.main()
