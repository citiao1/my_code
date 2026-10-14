import unittest

from vision.detector import Detection
from vision.face_tracker import OffsetSmoother, calculate_offset, intersection_over_union


class FaceTrackerTests(unittest.TestCase):
    def test_iou_for_identical_boxes(self) -> None:
        self.assertEqual(
            intersection_over_union((10, 10, 20, 20), (10, 10, 20, 20)),
            1.0,
        )

    def test_offset_uses_right_and_down_as_positive(self) -> None:
        offset = calculate_offset(
            Detection("face", 0.9, (180, 130, 40, 40)),
            320,
            240,
        )
        self.assertEqual(offset.face_center, (200, 150))
        self.assertEqual(offset.screen_center, (160, 120))
        self.assertEqual(offset.offset_px, (40, 30))

    def test_smoother_reduces_a_jump(self) -> None:
        smoother = OffsetSmoother(alpha=0.25)
        first = calculate_offset(
            Detection("face", 0.9, (140, 110, 40, 40)),
            320,
            240,
        )
        second = calculate_offset(
            Detection("face", 0.9, (220, 170, 40, 40)),
            320,
            240,
        )
        smoother.update(first)
        filtered = smoother.update(second)
        self.assertIsNotNone(filtered)
        assert filtered is not None
        self.assertEqual(filtered.offset_px, (20, 25))


if __name__ == "__main__":
    unittest.main()
