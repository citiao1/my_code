import unittest

from vision.face_tracker import FaceOffset
from vision.servo_tracker import FaceServoController, ServoTrackerConfig


def make_offset(dx: int, dy: int) -> FaceOffset:
    return FaceOffset(
        frame_size=(320, 240),
        face_center=(160 + dx, 120 + dy),
        screen_center=(160, 120),
        offset_px=(dx, dy),
        offset_norm=(dx / 160.0, dy / 120.0),
    )


class ServoTrackerTests(unittest.TestCase):
    def make_controller(self) -> FaceServoController:
        return FaceServoController(
            {"pan": 1500, "tilt": 1500},
            {"pan": (500, 1500, 2500), "tilt": (500, 1500, 2500)},
            ServoTrackerConfig(
                deadband_px=10,
                max_step_us=20,
                return_step_us=10,
                lost_hold_seconds=1.0,
                pan_invert=False,
                tilt_invert=False,
            ),
        )

    def test_deadband_does_not_move(self) -> None:
        controller = self.make_controller()
        self.assertEqual(
            controller.update(make_offset(10, -10)),
            {"pan": 1500, "tilt": 1500},
        )

    def test_error_moves_pan_and_tilt(self) -> None:
        controller = self.make_controller()
        pulses = controller.update(make_offset(100, 60))
        self.assertLess(pulses["pan"], 1500)
        self.assertLess(pulses["tilt"], 1500)

    def test_lost_target_returns_to_center_after_hold(self) -> None:
        controller = self.make_controller()
        controller.update(make_offset(100, 0))
        before = controller.update(None, now=10.0)
        after = controller.update(None, now=11.1)
        self.assertLess(before["pan"], 1500)
        self.assertGreater(after["pan"], before["pan"])


if __name__ == "__main__":
    unittest.main()
