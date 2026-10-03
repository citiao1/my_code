import unittest

import numpy as np

from vision.detector import Detection
from vision.run_face_tracking import resize_for_inference, scale_detections


class FaceDetectionHelperTests(unittest.TestCase):
    def test_resize_preserves_aspect_ratio_and_returns_scale(self) -> None:
        frame = np.zeros((480, 640, 3), dtype=np.uint8)
        resized, scale_x, scale_y = resize_for_inference(frame, 320)

        self.assertEqual(resized.shape[:2], (240, 320))
        self.assertEqual((scale_x, scale_y), (2.0, 2.0))

    def test_scale_detection_returns_full_frame_coordinates(self) -> None:
        detections = scale_detections(
            [Detection("face", 0.91, (10, 20, 30, 40))],
            2.0,
            2.0,
        )

        self.assertEqual(detections[0].bbox, (20, 40, 60, 80))
