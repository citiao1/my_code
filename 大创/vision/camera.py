from __future__ import annotations

import os
import time
from dataclasses import dataclass
from threading import Condition, Lock, Thread
from typing import Any

import cv2
import numpy as np


@dataclass(frozen=True)
class CameraConfig:
    source: str | int
    reconnect_delay: float = 2.0
    backend: int = cv2.CAP_FFMPEG


class ReconnectingCamera:
    """OpenCV source wrapper that keeps reconnecting after a stream failure."""

    def __init__(self, config: CameraConfig) -> None:
        self.config = config
        self._capture: cv2.VideoCapture | None = None

    def _open(self) -> bool:
        self.release()
        backend = cv2.CAP_ANY if isinstance(self.config.source, int) else self.config.backend
        capture = cv2.VideoCapture(self.config.source, backend)
        if not capture.isOpened():
            capture.release()
            return False
        self._capture = capture
        return True

    def read(self) -> tuple[bool, np.ndarray | None]:
        while True:
            if self._capture is None and not self._open():
                time.sleep(self.config.reconnect_delay)
                continue

            assert self._capture is not None
            ok, frame = self._capture.read()
            if ok and frame is not None:
                return True, frame

            self.release()
            time.sleep(self.config.reconnect_delay)

    def properties(self) -> dict[str, Any]:
        if self._capture is None:
            return {}
        return {
            "width": int(self._capture.get(cv2.CAP_PROP_FRAME_WIDTH)),
            "height": int(self._capture.get(cv2.CAP_PROP_FRAME_HEIGHT)),
            "fps": self._capture.get(cv2.CAP_PROP_FPS),
        }

    def release(self) -> None:
        if self._capture is not None:
            self._capture.release()
            self._capture = None


class LatestFrameCamera:
    """Continuously capture and retain only the newest frame.

    This prevents a slow detector from processing an ever-growing backlog of
    old frames. The consumer blocks until a newer frame is available, while
    the capture thread keeps draining OpenCV's input buffer.
    """

    def __init__(self, config: CameraConfig) -> None:
        self.config = config
        self._condition = Condition(Lock())
        self._capture: cv2.VideoCapture | None = None
        self._latest_frame: np.ndarray | None = None
        self._sequence = 0
        self._consumed_sequence = 0
        self._stop = False
        self._thread = Thread(
            target=self._capture_loop,
            name="latest-frame-capture",
            daemon=True,
        )
        self._thread.start()

    def _open(self) -> bool:
        self._close_capture()
        if self.config.backend == cv2.CAP_FFMPEG:
            transport = os.environ.get("VISION_RTSP_TRANSPORT", "udp")
            os.environ.setdefault(
                "OPENCV_FFMPEG_CAPTURE_OPTIONS",
                f"rtsp_transport;{transport}|fflags;nobuffer|flags;low_delay|"
                "max_delay;0|reorder_queue_size;0",
            )
        backend = cv2.CAP_ANY if isinstance(self.config.source, int) else self.config.backend
        capture = cv2.VideoCapture(self.config.source, backend)
        capture.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        if not capture.isOpened():
            capture.release()
            return False
        self._capture = capture
        return True

    def _close_capture(self) -> None:
        if self._capture is not None:
            self._capture.release()
            self._capture = None

    def _capture_loop(self) -> None:
        while True:
            with self._condition:
                if self._stop:
                    break
            if self._capture is None and not self._open():
                time.sleep(self.config.reconnect_delay)
                continue

            assert self._capture is not None
            ok, frame = self._capture.read()
            if not ok or frame is None:
                self._close_capture()
                time.sleep(self.config.reconnect_delay)
                continue

            with self._condition:
                self._latest_frame = frame
                self._sequence += 1
                self._condition.notify_all()

        self._close_capture()

    def read(self) -> tuple[bool, np.ndarray | None]:
        with self._condition:
            while not self._stop and self._sequence <= self._consumed_sequence:
                self._condition.wait(timeout=self.config.reconnect_delay)
            if self._latest_frame is None:
                return False, None
            self._consumed_sequence = self._sequence
            return True, self._latest_frame

    def properties(self) -> dict[str, Any]:
        if self._capture is None:
            return {}
        return {
            "width": int(self._capture.get(cv2.CAP_PROP_FRAME_WIDTH)),
            "height": int(self._capture.get(cv2.CAP_PROP_FRAME_HEIGHT)),
            "fps": self._capture.get(cv2.CAP_PROP_FPS),
        }

    def release(self) -> None:
        with self._condition:
            self._stop = True
            self._condition.notify_all()
        self._thread.join(timeout=max(self.config.reconnect_delay + 1.0, 2.0))


def source_value(source: str) -> str | int:
    """Treat a numeric source as a local camera index."""

    return int(source) if source.isdecimal() else source
