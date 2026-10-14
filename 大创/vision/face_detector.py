from __future__ import annotations

import os

import cv2
import numpy as np

try:
    from .detector import Detection
except ImportError:
    from detector import Detection


class YuNetFaceDetector:
    """OpenCV YuNet face detector with a small, stable application interface."""

    def __init__(
        self,
        model_path: str,
        score_threshold: float = 0.6,
        nms_threshold: float = 0.3,
        top_k: int = 5000,
    ) -> None:
        if not os.path.isfile(model_path):
            raise FileNotFoundError(
                f"YuNet model not found: {model_path}. "
                "Download face_detection_yunet_2023mar.onnx first."
            )

        create = getattr(cv2.FaceDetectorYN, "create", None)
        if create is None:
            create = cv2.FaceDetectorYN_create
        self.detector = create(
            model_path,
            "",
            (320, 320),
            score_threshold,
            nms_threshold,
            top_k,
        )
        self._input_size: tuple[int, int] | None = None

    def detect(self, frame: np.ndarray) -> list[Detection]:
        height, width = frame.shape[:2]
        input_size = (width, height)
        if input_size != self._input_size:
            self.detector.setInputSize(input_size)
            self._input_size = input_size
        _, faces = self.detector.detect(frame)
        if faces is None:
            return []

        detections: list[Detection] = []
        for face in faces:
            x, y, box_width, box_height = face[:4]
            confidence = face[-1]
            x1 = max(0, int(round(x)))
            y1 = max(0, int(round(y)))
            x2 = min(width, int(round(x + box_width)))
            y2 = min(height, int(round(y + box_height)))
            if x2 <= x1 or y2 <= y1:
                continue
            detections.append(
                Detection(
                    label="face",
                    confidence=float(confidence),
                    bbox=(x1, y1, x2 - x1, y2 - y1),
                )
            )
        return sorted(
            detections,
            key=lambda item: item.bbox[2] * item.bbox[3],
            reverse=True,
        )
