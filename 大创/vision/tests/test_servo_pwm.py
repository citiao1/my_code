import tempfile
import unittest
from pathlib import Path

from vision.servo_pwm import ServoLimits, SysfsPwm


class ServoPwmTests(unittest.TestCase):
    def make_exported_channel(self, root: Path) -> Path:
        chip = root / "pwmchip0"
        channel = chip / "pwm3"
        channel.mkdir(parents=True)
        for name, value in (
            ("enable", "0"),
            ("period", "0"),
            ("duty_cycle", "0"),
            ("polarity", "normal"),
        ):
            (channel / name).write_text(value, encoding="ascii")
        return chip

    def test_open_sets_safe_50hz_center_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            chip = self.make_exported_channel(Path(temporary))
            pwm = SysfsPwm(chip, channel=3)
            pwm.open(1_500_000)
            self.assertEqual(
                (chip / "pwm3" / "period").read_text(encoding="ascii").strip(),
                "20000000",
            )
            self.assertEqual(
                (chip / "pwm3" / "duty_cycle").read_text(encoding="ascii").strip(),
                "1500000",
            )
            self.assertEqual(
                (chip / "pwm3" / "enable").read_text(encoding="ascii").strip(),
                "1",
            )
            pwm.close()
            self.assertEqual(
                (chip / "pwm3" / "enable").read_text(encoding="ascii").strip(),
                "0",
            )

    def test_limits_reject_pulses_outside_initial_safe_range(self) -> None:
        limits = ServoLimits()
        limits.validate(1000)
        limits.validate(2000)
        with self.assertRaises(ValueError):
            limits.validate(999)
        with self.assertRaises(ValueError):
            limits.validate(2001)


if __name__ == "__main__":
    unittest.main()
