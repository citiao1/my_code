import unittest

import cv2
import numpy as np

from vision.detector import ColorBlobDetector, NoopDetector, draw_detections


class DetectorTests(unittest.TestCase):
    def test_noop_detector_is_safe(self) -> None:
        frame = np.zeros((100, 120, 3), dtype=np.uint8)
        self.assertEqual(NoopDetector().detect(frame), [])

    def test_color_detector_finds_large_orange_region(self) -> None:
        frame = np.zeros((240, 320, 3), dtype=np.uint8)
        cv2.rectangle(frame, (80, 60), (220, 180), (0, 100, 255), -1)

        detections = ColorBlobDetector(min_area=500).detect(frame)

        self.assertEqual(len(detections), 1)
        self.assertGreaterEqual(detections[0].bbox[2], 130)
        self.assertGreaterEqual(detections[0].bbox[3], 110)
        self.assertEqual(detections[0].center, (150, 120))

    def test_draw_detections_does_not_mutate_input(self) -> None:
        frame = np.zeros((80, 100, 3), dtype=np.uint8)
        cv2.rectangle(frame, (10, 10), (60, 60), (0, 100, 255), -1)
        original = frame.copy()
        detection = ColorBlobDetector(min_area=1).detect(frame)

        annotated = draw_detections(frame, detection)

        np.testing.assert_array_equal(frame, original)
        self.assertFalse(np.array_equal(annotated, frame))


if __name__ == "__main__":
    unittest.main()
