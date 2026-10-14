from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol

import cv2
import numpy as np


@dataclass(frozen=True)
class Detection:
    """One detector result in pixel coordinates."""

    label: str
    confidence: float
    bbox: tuple[int, int, int, int]

    @property
    def center(self) -> tuple[int, int]:
        x, y, width, height = self.bbox
        return x + width // 2, y + height // 2


class Detector(Protocol):
    def detect(self, frame: np.ndarray) -> list[Detection]:
        """Return detections for one BGR frame."""


class NoopDetector:
    """A safe default that never produces a control-worthy detection."""

    def detect(self, frame: np.ndarray) -> list[Detection]:
        del frame
        return []


class ColorBlobDetector:
    """Simple HSV color detector for early camera and pipeline validation.

    This is deliberately not a final object-recognition model. It provides a
    deterministic way to validate the full video -> detection -> annotation
    pipeline before a trained model and STM32 output are introduced.
    """

    def __init__(
        self,
        lower_hsv: tuple[int, int, int] = (0, 100, 80),
        upper_hsv: tuple[int, int, int] = (15, 255, 255),
        min_area: int = 400,
        label: str = "color-target",
    ) -> None:
        self.lower_hsv = np.array(lower_hsv, dtype=np.uint8)
        self.upper_hsv = np.array(upper_hsv, dtype=np.uint8)
        self.min_area = min_area
        self.label = label

    def detect(self, frame: np.ndarray) -> list[Detection]:
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        mask = cv2.inRange(hsv, self.lower_hsv, self.upper_hsv)
        kernel = np.ones((5, 5), dtype=np.uint8)
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)

        contours, _ = cv2.findContours(
            mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
        )
        detections: list[Detection] = []
        for contour in contours:
            area = cv2.contourArea(contour)
            if area < self.min_area:
                continue
            x, y, width, height = cv2.boundingRect(contour)
            frame_area = max(frame.shape[0] * frame.shape[1], 1)
            confidence = min(area / frame_area * 8.0, 0.99)
            detections.append(
                Detection(
                    label=self.label,
                    confidence=confidence,
                    bbox=(x, y, width, height),
                )
            )
        return sorted(detections, key=lambda item: item.bbox[2] * item.bbox[3], reverse=True)


def draw_detections(
    frame: np.ndarray,
    detections: list[Detection],
) -> np.ndarray:
    """Draw results on a copy so the raw frame remains untouched."""

    annotated = frame.copy()
    for detection in detections:
        x, y, width, height = detection.bbox
        color = (0, 220, 0)
        cv2.rectangle(annotated, (x, y), (x + width, y + height), color, 2)
        center_x, center_y = detection.center
        cv2.circle(annotated, (center_x, center_y), 4, (0, 0, 255), -1)
        text = f"{detection.label} {detection.confidence:.2f}"
        cv2.putText(
            annotated,
            text,
            (x, max(y - 8, 20)),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.55,
            color,
            2,
            cv2.LINE_AA,
        )
    return annotated


def create_detector(name: str) -> Detector:
    if name == "none":
        return NoopDetector()
    if name == "color":
        return ColorBlobDetector()
    raise ValueError(f"unknown detector: {name}")
