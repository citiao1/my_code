from __future__ import annotations

import time
from dataclasses import dataclass

try:
    from .face_tracker import FaceOffset
except ImportError:
    from face_tracker import FaceOffset


@dataclass(frozen=True)
class ServoTrackerConfig:
    control_mode: str = "pid"
    deadband_px: int = 5
    max_step_us: int = 50
    return_step_us: int = 5
    lost_hold_seconds: float = 1.5
    pan_invert: bool = False
    tilt_invert: bool = False
    kp: float = 500.0
    ki: float = 0.0
    kd: float = 200.0
    integral_limit: float = 0.5
    derivative_alpha: float = 0.35


class FaceServoController:
    """Convert smoothed face offsets into bounded, rate-limited servo pulses."""

    def __init__(
        self,
        initial_pulses: dict[str, int],
        limits: dict[str, tuple[int, int, int]],
        config: ServoTrackerConfig | None = None,
    ) -> None:
        self.config = config or ServoTrackerConfig()
        self.limits = limits
        self.pulses = dict(initial_pulses)
        self._lost_since: float | None = None
        self._integrals = {"pan": 0.0, "tilt": 0.0}
        self._previous_errors: dict[str, float | None] = {
            "pan": None,
            "tilt": None,
        }
        self._filtered_derivatives = {"pan": 0.0, "tilt": 0.0}
        self._last_pid_time: float | None = None

    def reset(self, pulses: dict[str, int]) -> None:
        self.pulses = dict(pulses)
        self._lost_since = None
        self._integrals = {"pan": 0.0, "tilt": 0.0}
        self._previous_errors = {"pan": None, "tilt": None}
        self._filtered_derivatives = {"pan": 0.0, "tilt": 0.0}
        self._last_pid_time = None

    def set_config(self, config: ServoTrackerConfig) -> None:
        self.config = config

    def _clamp(self, axis: str, pulse_us: int) -> int:
        minimum, _, maximum = self.limits[axis]
        return max(minimum, min(maximum, pulse_us))

    def _track_axis(
        self,
        axis: str,
        error_px: int,
        extent_px: int,
        invert: bool,
    ) -> None:
        if abs(error_px) <= self.config.deadband_px:
            return
        available = max(extent_px // 2 - self.config.deadband_px, 1)
        magnitude = min(
            (abs(error_px) - self.config.deadband_px) / available,
            1.0,
        )
        step = max(1, round(self.config.max_step_us * magnitude))
        direction = 1 if error_px > 0 else -1
        if invert:
            direction *= -1
        self.pulses[axis] = self._clamp(
            axis,
            self.pulses[axis] + direction * step,
        )

    def _pid_axis(
        self,
        axis: str,
        error_px: int,
        extent_px: int,
        invert: bool,
        dt: float,
    ) -> None:
        if abs(error_px) <= self.config.deadband_px:
            self._integrals[axis] = 0.0
            self._previous_errors[axis] = 0.0
            self._filtered_derivatives[axis] = 0.0
            return

        half_extent = max(extent_px / 2.0, 1.0)
        error = max(-1.0, min(1.0, error_px / half_extent))
        previous = self._previous_errors[axis]
        derivative = 0.0 if previous is None else (error - previous) / dt
        alpha = self.config.derivative_alpha
        filtered_derivative = (
            self._filtered_derivatives[axis]
            + alpha * (derivative - self._filtered_derivatives[axis])
        )
        integral = self._integrals[axis] + error * dt
        integral = max(
            -self.config.integral_limit,
            min(self.config.integral_limit, integral),
        )
        output_rate = (
            self.config.kp * error
            + self.config.ki * integral
            + self.config.kd * filtered_derivative
        )
        if invert:
            output_rate *= -1.0
        step = round(output_rate * dt)
        if step == 0:
            step = 1 if output_rate > 0 else -1
        step = max(-self.config.max_step_us, min(self.config.max_step_us, step))

        self._integrals[axis] = integral
        self._previous_errors[axis] = error
        self._filtered_derivatives[axis] = filtered_derivative
        self.pulses[axis] = self._clamp(axis, self.pulses[axis] + step)

    def _return_axis(self, axis: str) -> None:
        _, center, _ = self.limits[axis]
        current = self.pulses[axis]
        if current < center:
            current += min(self.config.return_step_us, center - current)
        elif current > center:
            current -= min(self.config.return_step_us, current - center)
        self.pulses[axis] = self._clamp(axis, current)

    def update(
        self,
        offset: FaceOffset | None,
        now: float | None = None,
    ) -> dict[str, int]:
        timestamp = time.monotonic() if now is None else now
        if offset is not None:
            self._lost_since = None
            frame_width, frame_height = offset.frame_size
            # Camera-view convention: left and up are positive.
            horizontal_error = -offset.offset_px[0]
            vertical_error = -offset.offset_px[1]
            if self._last_pid_time is None:
                dt = 1.0 / 20.0
            else:
                dt = max(timestamp - self._last_pid_time, 1e-3)
            self._last_pid_time = timestamp
            if self.config.control_mode == "pid":
                self._pid_axis(
                    "pan",
                    horizontal_error,
                    frame_width,
                    self.config.pan_invert,
                    dt,
                )
                self._pid_axis(
                    "tilt",
                    vertical_error,
                    frame_height,
                    self.config.tilt_invert,
                    dt,
                )
            else:
                self._track_axis(
                    "pan",
                    horizontal_error,
                    frame_width,
                    self.config.pan_invert,
                )
                self._track_axis(
                    "tilt",
                    vertical_error,
                    frame_height,
                    self.config.tilt_invert,
                )
            return dict(self.pulses)

        if self._lost_since is None:
            self._lost_since = timestamp
        self._last_pid_time = None
        self._previous_errors = {"pan": None, "tilt": None}
        self._filtered_derivatives = {"pan": 0.0, "tilt": 0.0}
        if timestamp - self._lost_since >= self.config.lost_hold_seconds:
            self._return_axis("pan")
            self._return_axis("tilt")
        return dict(self.pulses)
