import unittest
from types import SimpleNamespace

from vision.vision_worker import FaceTrackingWorker


class FakeRig:
    def __init__(self) -> None:
        limits = SimpleNamespace(min_us=500, center_us=1500, max_us=2500)
        self.axes = {
            "pan": SimpleNamespace(
                config=SimpleNamespace(limits=limits),
                pulse_us=1500,
            ),
            "tilt": SimpleNamespace(
                config=SimpleNamespace(limits=limits),
                pulse_us=1500,
            ),
        }


class VisionWorkerTests(unittest.TestCase):
    def test_tracking_config_can_be_updated_at_runtime(self) -> None:
        worker = FaceTrackingWorker(FakeRig())
        updated = worker.set_tracking_config(
            {
                "deadband_px": 8,
                "max_step_us": 24,
                "return_step_us": 7,
                "smooth_alpha": 0.6,
                "detect_every": 1,
                "max_fps": 20,
            }
        )
        self.assertEqual(updated["deadband_px"], 8)
        self.assertEqual(updated["max_step_us"], 24)
        self.assertEqual(updated["return_step_us"], 7)
        self.assertEqual(updated["smooth_alpha"], 0.6)
        self.assertEqual(updated["detect_every"], 1)
        self.assertEqual(updated["max_fps"], 20.0)

    def test_tracking_config_rejects_unsafe_values(self) -> None:
        worker = FaceTrackingWorker(FakeRig())
        with self.assertRaises(ValueError):
            worker.set_tracking_config({"max_step_us": 0})
        with self.assertRaises(ValueError):
            worker.set_tracking_config({"smooth_alpha": 1.1})

    def test_detection_schedule_includes_every_frame_when_interval_is_one(self) -> None:
        self.assertTrue(FaceTrackingWorker.should_detect(1, 1))
        self.assertTrue(FaceTrackingWorker.should_detect(2, 1))
        self.assertTrue(FaceTrackingWorker.should_detect(3, 1))
        self.assertTrue(FaceTrackingWorker.should_detect(1, 2))
        self.assertFalse(FaceTrackingWorker.should_detect(2, 2))
        self.assertTrue(FaceTrackingWorker.should_detect(3, 2))

    def test_state_can_update_event_without_replacing_preview(self) -> None:
        from vision.vision_worker import VisionState

        state = VisionState()
        state.update(b"jpeg", {"frame": 1})
        state.update_event({"frame": 2})
        self.assertEqual(state.frame(), b"jpeg")
        self.assertEqual(state.status()["frame"], 2)


if __name__ == "__main__":
    unittest.main()
