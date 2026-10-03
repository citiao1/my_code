import unittest

from vision.servo_server import ServoRig


class FakePwm:
    def __init__(self, **kwargs) -> None:
        self.channel = kwargs["channel"]
        self.is_open = False
        self.pulse_us = None

    def open(self, initial_pulse_ns: int) -> None:
        self.is_open = True
        self.pulse_us = initial_pulse_ns // 1000

    def set_pulse_us(self, pulse_us: int) -> None:
        if not self.is_open:
            raise RuntimeError("not open")
        self.pulse_us = pulse_us

    def stop(self) -> None:
        self.is_open = False

    def close(self) -> None:
        self.is_open = False


class ServoRigTests(unittest.TestCase):
    def make_rig(self) -> ServoRig:
        rig = ServoRig(pwm_factory=FakePwm, session_timeout=1.0)
        rig.start()
        return rig

    def test_start_centers_both_axes(self) -> None:
        rig = self.make_rig()
        status = rig.status()
        self.assertEqual(status["axes"]["pan"]["pulse_us"], 1500)
        self.assertEqual(status["axes"]["tilt"]["pulse_us"], 1500)
        self.assertTrue(status["pwm_running"])
        rig.close()

    def test_session_can_nudge_and_clamps_to_safe_limits(self) -> None:
        rig = self.make_rig()
        token = rig.create_session()
        self.assertEqual(rig.command(token, "pan", delta_us=-1200), 500)
        self.assertEqual(rig.command(token, "tilt", delta_us=1200), 2500)
        rig.close()

    def test_full_range_maps_to_minus90_and_plus90_degrees(self) -> None:
        rig = self.make_rig()
        token = rig.create_session()
        rig.command(token, "pan", pulse_us=500)
        self.assertEqual(rig.status()["axes"]["pan"]["angle_deg"], -90.0)
        rig.command(token, "pan", pulse_us=2500)
        self.assertEqual(rig.status()["axes"]["pan"]["angle_deg"], 90.0)
        rig.close()

    def test_watchdog_stops_after_heartbeat_timeout(self) -> None:
        rig = ServoRig(pwm_factory=FakePwm, session_timeout=1.0)
        rig.start()
        rig.create_session()
        self.assertTrue(rig.check_watchdog(now=rig._last_heartbeat + 1.1))
        self.assertFalse(rig.status()["pwm_running"])
        rig.close()

    def test_new_session_reopens_pwm_after_stop(self) -> None:
        rig = self.make_rig()
        token = rig.create_session()
        rig.stop()
        self.assertFalse(rig.status()["pwm_running"])
        next_token = rig.create_session()
        self.assertNotEqual(token, next_token)
        self.assertTrue(rig.status()["pwm_running"])
        self.assertEqual(rig.status()["axes"]["pan"]["pulse_us"], 1500)
        rig.close()


if __name__ == "__main__":
    unittest.main()
