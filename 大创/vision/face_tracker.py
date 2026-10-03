from __future__ import annotations

from dataclasses import dataclass

try:
    from .detector import Detection
except ImportError:
    from detector import Detection


def intersection_over_union(
    first: tuple[int, int, int, int],
    second: tuple[int, int, int, int],
) -> float:
    def area(box: tuple[int, int, int, int]) -> int:
        return max(box[2], 0) * max(box[3], 0)

    first_x, first_y, first_w, first_h = first
    second_x, second_y, second_w, second_h = second
    left = max(first_x, second_x)
    top = max(first_y, second_y)
    right = min(first_x + first_w, second_x + second_w)
    bottom = min(first_y + first_h, second_y + second_h)
    intersection = area((left, top, right - left, bottom - top))
    union = area(first) + area(second) - intersection
    return intersection / union if union else 0.0


@dataclass(frozen=True)
class FaceOffset:
    frame_size: tuple[int, int]
    face_center: tuple[int, int]
    screen_center: tuple[int, int]
    offset_px: tuple[int, int]
    offset_norm: tuple[float, float]


class OffsetSmoother:
    """Low-pass filter for face-center coordinates and screen offsets."""

    def __init__(self, alpha: float = 0.35) -> None:
        if not 0.0 < alpha <= 1.0:
            raise ValueError("alpha must be in (0, 1]")
        self.alpha = alpha
        self._value: FaceOffset | None = None

    def set_alpha(self, alpha: float) -> None:
        if not 0.0 < alpha <= 1.0:
            raise ValueError("alpha must be in (0, 1]")
        self.alpha = alpha

    def update(self, value: FaceOffset | None) -> FaceOffset | None:
        if value is None:
            self._value = None
            return None
        if self._value is None:
            self._value = value
            return value

        old = self._value

        def smooth(previous: float, current: float) -> float:
            return previous + self.alpha * (current - previous)

        face_center = (
            round(smooth(old.face_center[0], value.face_center[0])),
            round(smooth(old.face_center[1], value.face_center[1])),
        )
        offset_px = (
            round(smooth(old.offset_px[0], value.offset_px[0])),
            round(smooth(old.offset_px[1], value.offset_px[1])),
        )
        offset_norm = (
            smooth(old.offset_norm[0], value.offset_norm[0]),
            smooth(old.offset_norm[1], value.offset_norm[1]),
        )
        self._value = FaceOffset(
            frame_size=value.frame_size,
            face_center=face_center,
            screen_center=value.screen_center,
            offset_px=offset_px,
            offset_norm=offset_norm,
        )
        return self._value


class FaceTargetTracker:
    """Keep one target face stable when multiple faces are visible."""

    def __init__(self, minimum_iou: float = 0.05) -> None:
        self.minimum_iou = minimum_iou
        self.previous: Detection | None = None

    def select(self, detections: list[Detection]) -> Detection | None:
        if not detections:
            self.previous = None
            return None

        if self.previous is None:
            selected = detections[0]
        else:
            selected = max(
                detections,
                key=lambda item: intersection_over_union(
                    self.previous.bbox,
                    item.bbox,
                ),
            )
            if intersection_over_union(self.previous.bbox, selected.bbox) < self.minimum_iou:
                selected = detections[0]

        self.previous = selected
        return selected


def calculate_offset(
    detection: Detection,
    frame_width: int,
    frame_height: int,
) -> FaceOffset:
    face_center = detection.center
    screen_center = (frame_width // 2, frame_height // 2)
    offset_x = face_center[0] - screen_center[0]
    offset_y = face_center[1] - screen_center[1]
    half_width = max(frame_width / 2.0, 1.0)
    half_height = max(frame_height / 2.0, 1.0)
    return FaceOffset(
        frame_size=(frame_width, frame_height),
        face_center=face_center,
        screen_center=screen_center,
        offset_px=(offset_x, offset_y),
        offset_norm=(offset_x / half_width, offset_y / half_height),
    )
