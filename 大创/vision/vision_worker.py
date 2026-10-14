from __future__ import annotations

import os
import time
from dataclasses import dataclass, replace
from threading import Event, Lock, RLock, Thread
from typing import Any

import cv2

try:
    from .camera import CameraConfig, LatestFrameCamera
    from .detector import Detection
    from .face_detector import YuNetFaceDetector
    from .face_tracker import FaceTargetTracker, OffsetSmoother, calculate_offset
    from .run_face_tracking import draw_face_tracking, resize_for_inference, scale_detections
    from .servo_tracker import FaceServoController, ServoTrackerConfig
except ImportError:
    from camera import CameraConfig, LatestFrameCamera
    from detector import Detection
    from face_detector import YuNetFaceDetector
    from face_tracker import FaceTargetTracker, OffsetSmoother, calculate_offset
    from run_face_tracking import draw_face_tracking, resize_for_inference, scale_detections
    from servo_tracker import FaceServoController, ServoTrackerConfig


@dataclass(frozen=True)
class VisionConfig:
    source: str = "rtsp://127.0.0.1:8554/camera"
    model: str = "vision/models/face_detection_yunet_2023mar.onnx"
    confidence: float = 0.6
    inference_width: int = 320
    detect_every: int = 1
    smooth_alpha: float = 0.35
    max_fps: float = 30.0
    preview_fps: float = 8.0


class VisionState:
    def __init__(self) -> None:
        self._lock = Lock()
        self._jpeg = b""
        self._event: dict[str, Any] = {
            "status": "starting",
            "auto_enabled": False,
        }

    def update(self, jpeg: bytes, event: dict[str, Any]) -> None:
        with self._lock:
            self._jpeg = jpeg
            self._event = dict(event)

    def update_event(self, event: dict[str, Any]) -> None:
        with self._lock:
            self._event = dict(event)

    def set_auto_enabled(self, enabled: bool) -> None:
        with self._lock:
            self._event["auto_enabled"] = enabled

    def status(self) -> dict[str, Any]:
        with self._lock:
            return dict(self._event)

    def frame(self) -> bytes:
        with self._lock:
            return self._jpeg


class FaceTrackingWorker:
    """Capture, detect, annotate, and optionally drive the two-servo rig."""

    def __init__(
        self,
        rig: Any,
        config: VisionConfig | None = None,
        tracker_config: ServoTrackerConfig | None = None,
    ) -> None:
        self.rig = rig
        self.config = config or VisionConfig()
        self.state = VisionState()
        self._tracker_config = tracker_config or ServoTrackerConfig()
        self._tuning_lock = RLock()
        self._smooth_alpha = self.config.smooth_alpha
        self._detect_every = self.config.detect_every
        self._max_fps = self.config.max_fps
        self._auto_lock = Lock()
        self._auto_enabled = False
        self._stop = Event()
        self._thread = Thread(
            target=self._run,
            name="face-servo-tracking",
            daemon=True,
        )

        limits = {
            name: (
                axis.config.limits.min_us,
                axis.config.limits.center_us,
                axis.config.limits.max_us,
            )
            for name, axis in rig.axes.items()
        }
        initial = {name: axis.pulse_us for name, axis in rig.axes.items()}
        self._controller = FaceServoController(
            initial,
            limits,
            self._tracker_config,
        )

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        self._thread.join(timeout=4.0)

    def set_auto_enabled(self, enabled: bool) -> None:
        with self._auto_lock:
            self._auto_enabled = enabled
        self.state.set_auto_enabled(enabled)
        if enabled:
            self.rig.start()
            self._controller.reset(
                {name: axis.pulse_us for name, axis in self.rig.axes.items()}
            )

    def auto_enabled(self) -> bool:
        with self._auto_lock:
            return self._auto_enabled

    def status(self) -> dict[str, Any]:
        event = self.state.status()
        event["auto_enabled"] = self.auto_enabled()
        event["tracking_config"] = self.tracking_config()
        return event

    def frame(self) -> bytes:
        return self.state.frame()

    def tracking_config(self) -> dict[str, Any]:
        with self._tuning_lock:
            config = self._tracker_config
            return {
                "control_mode": config.control_mode,
                "deadband_px": config.deadband_px,
                "max_step_us": config.max_step_us,
                "return_step_us": config.return_step_us,
                "kp": config.kp,
                "ki": config.ki,
                "kd": config.kd,
                "integral_limit": config.integral_limit,
                "derivative_alpha": config.derivative_alpha,
                "smooth_alpha": self._smooth_alpha,
                "detect_every": self._detect_every,
                "max_fps": self._max_fps,
            }

    def set_tracking_config(self, values: dict[str, Any]) -> dict[str, Any]:
        with self._tuning_lock:
            current = self._tracker_config
            control_mode = values.get("control_mode", current.control_mode)
            if control_mode not in {"p", "pid"}:
                raise ValueError("control_mode must be p or pid")
            updated = replace(
                current,
                control_mode=control_mode,
                deadband_px=self._read_int(
                    values,
                    "deadband_px",
                    current.deadband_px,
                    minimum=0,
                    maximum=100,
                ),
                max_step_us=self._read_int(
                    values,
                    "max_step_us",
                    current.max_step_us,
                    minimum=1,
                    maximum=50,
                ),
                return_step_us=self._read_int(
                    values,
                    "return_step_us",
                    current.return_step_us,
                    minimum=1,
                    maximum=30,
                ),
                kp=self._read_float(values, "kp", current.kp, 0.0, 3000.0),
                ki=self._read_float(values, "ki", current.ki, 0.0, 1000.0),
                kd=self._read_float(values, "kd", current.kd, 0.0, 500.0),
                integral_limit=self._read_float(
                    values,
                    "integral_limit",
                    current.integral_limit,
                    minimum=0.05,
                    maximum=2.0,
                ),
                derivative_alpha=self._read_float(
                    values,
                    "derivative_alpha",
                    current.derivative_alpha,
                    minimum=0.05,
                    maximum=1.0,
                ),
            )
            smooth_alpha = self._read_float(
                values,
                "smooth_alpha",
                self._smooth_alpha,
                minimum=0.05,
                maximum=1.0,
            )
            detect_every = self._read_int(
                values,
                "detect_every",
                self._detect_every,
                minimum=1,
                maximum=4,
            )
            max_fps = self._read_float(
                values,
                "max_fps",
                self._max_fps,
                minimum=5.0,
                maximum=30.0,
            )
            self._tracker_config = updated
            self._controller.set_config(updated)
            self._smooth_alpha = smooth_alpha
            self._detect_every = detect_every
            self._max_fps = max_fps
            return self.tracking_config()

    @staticmethod
    def should_detect(frame_count: int, detect_every: int) -> bool:
        interval = max(int(detect_every), 1)
        return frame_count >= 1 and (frame_count - 1) % interval == 0

    @staticmethod
    def _read_int(
        values: dict[str, Any],
        name: str,
        default: int,
        minimum: int,
        maximum: int,
    ) -> int:
        if name not in values:
            return default
        value = values[name]
        if isinstance(value, bool):
            raise ValueError(f"{name} must be an integer")
        try:
            parsed = int(value)
        except (TypeError, ValueError) as exc:
            raise ValueError(f"{name} must be an integer") from exc
        if parsed < minimum or parsed > maximum:
            raise ValueError(f"{name} must be between {minimum} and {maximum}")
        return parsed

    @staticmethod
    def _read_float(
        values: dict[str, Any],
        name: str,
        default: float,
        minimum: float,
        maximum: float,
    ) -> float:
        if name not in values:
            return default
        value = values[name]
        if isinstance(value, bool):
            raise ValueError(f"{name} must be a number")
        try:
            parsed = float(value)
        except (TypeError, ValueError) as exc:
            raise ValueError(f"{name} must be a number") from exc
        if parsed < minimum or parsed > maximum:
            raise ValueError(f"{name} must be between {minimum} and {maximum}")
        return parsed

    def _run(self) -> None:
        camera = LatestFrameCamera(
            CameraConfig(
                source=self.config.source,
                reconnect_delay=1.0,
            )
        )
        try:
            detector = YuNetFaceDetector(
                os.path.abspath(self.config.model),
                score_threshold=self.config.confidence,
            )
            self._process(camera, detector)
        except Exception as exc:
            self.state.update(
                b"",
                {
                    "status": "error",
                    "error": f"{type(exc).__name__}: {exc}",
                    "auto_enabled": self.auto_enabled(),
                },
            )
        finally:
            camera.release()

    def _process(
        self,
        camera: LatestFrameCamera,
        detector: YuNetFaceDetector,
    ) -> None:
        target_tracker = FaceTargetTracker()
        smoother = OffsetSmoother(self._smooth_alpha)
        last_frame_time = time.monotonic()
        smoothed_fps = 0.0
        frame_count = 0
        selected: Detection | None = None
        next_frame_time = time.monotonic()
        next_preview_time = time.monotonic()

        while not self._stop.is_set():
            ok, frame = camera.read()
            if not ok or frame is None:
                continue

            with self._tuning_lock:
                smooth_alpha = self._smooth_alpha
                detect_every = self._detect_every
                max_fps = self._max_fps
            smoother.set_alpha(smooth_alpha)

            now = time.monotonic()
            delta = max(now - last_frame_time, 1e-6)
            instant_fps = 1.0 / delta
            smoothed_fps = (
                instant_fps
                if smoothed_fps == 0
                else smoothed_fps * 0.9 + instant_fps * 0.1
            )
            last_frame_time = now
            frame_count += 1

            if self.should_detect(frame_count, detect_every):
                inference_frame, scale_x, scale_y = resize_for_inference(
                    frame,
                    self.config.inference_width,
                )
                detections = scale_detections(
                    detector.detect(inference_frame),
                    scale_x,
                    scale_y,
                )
                selected = target_tracker.select(detections)
            else:
                selected = target_tracker.previous

            raw_offset = (
                calculate_offset(selected, frame.shape[1], frame.shape[0])
                if selected is not None
                else None
            )
            offset = smoother.update(raw_offset)
            if self.auto_enabled():
                pulses = self._controller.update(offset, now)
                for name, pulse_us in pulses.items():
                    self.rig.set_internal_pulse(name, pulse_us)

            event = {
                "timestamp": time.time(),
                "frame": frame_count,
                "fps": round(smoothed_fps, 2),
                "status": "face" if selected is not None else "no_face",
                "auto_enabled": self.auto_enabled(),
                "bbox": selected.bbox if selected is not None else None,
                "confidence": (
                    round(selected.confidence, 4)
                    if selected is not None
                    else None
                ),
                "face_center": offset.face_center if offset else None,
                "screen_center": offset.screen_center if offset else None,
                # Camera-view convention: left and up are positive.
                "offset_px": (
                    (-offset.offset_px[0], -offset.offset_px[1])
                    if offset
                    else None
                ),
                "offset_px_image": offset.offset_px if offset else None,
                "offset_norm": (
                    tuple(round(-value, 5) for value in offset.offset_norm)
                    if offset
                    else None
                ),
            }
            if self.config.preview_fps <= 0 or now >= next_preview_time:
                annotated = draw_face_tracking(frame, selected, offset, smoothed_fps)
                encoded_ok, encoded = cv2.imencode(
                    ".jpg",
                    annotated,
                    [cv2.IMWRITE_JPEG_QUALITY, 75],
                )
                if encoded_ok:
                    self.state.update(encoded.tobytes(), event)
                else:
                    self.state.update_event(event)
                if self.config.preview_fps > 0:
                    next_preview_time = now + 1.0 / self.config.preview_fps
            else:
                self.state.update_event(event)

            if max_fps > 0:
                next_frame_time += 1.0 / max_fps
                wait_seconds = next_frame_time - time.monotonic()
                if wait_seconds > 0:
                    self._stop.wait(wait_seconds)
                else:
                    next_frame_time = time.monotonic()
