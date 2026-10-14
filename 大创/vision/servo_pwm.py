from __future__ import annotations

import time
from dataclasses import dataclass
from pathlib import Path


class PwmError(RuntimeError):
    """Raised when the Linux sysfs PWM channel cannot be controlled."""


@dataclass(frozen=True)
class ServoLimits:
    min_us: int = 1000
    center_us: int = 1500
    max_us: int = 2000

    def __post_init__(self) -> None:
        if not 0 < self.min_us <= self.center_us <= self.max_us:
            raise ValueError("servo limits must satisfy 0 < min <= center <= max")

    def validate(self, pulse_us: int) -> None:
        if not self.min_us <= pulse_us <= self.max_us:
            raise ValueError(
                f"pulse must be between {self.min_us} and {self.max_us} us: {pulse_us}"
            )


class SysfsPwm:
    """Small, safety-limited wrapper around the Linux PWM sysfs interface."""

    def __init__(
        self,
        chip_path: str | Path = "/sys/class/pwm/pwmchip0",
        channel: int = 3,
        period_ns: int = 20_000_000,
        export_timeout: float = 1.0,
    ) -> None:
        if channel < 0:
            raise ValueError("channel must be non-negative")
        if period_ns <= 0:
            raise ValueError("period_ns must be positive")
        self.chip_path = Path(chip_path)
        self.channel = channel
        self.period_ns = period_ns
        self.export_timeout = export_timeout
        self.pwm_path = self.chip_path / f"pwm{channel}"
        self._exported_by_us = False
        self._opened = False

    def _write(self, name: str, value: str | int) -> None:
        path = self.pwm_path / name
        try:
            path.write_text(f"{value}\n", encoding="ascii")
        except OSError as exc:
            raise PwmError(f"cannot write {path}: {exc}") from exc

    def _is_enabled(self) -> bool:
        try:
            return (self.pwm_path / "enable").read_text(encoding="ascii").strip() == "1"
        except OSError:
            return False

    def _wait_for_channel(self) -> None:
        deadline = time.monotonic() + self.export_timeout
        while not self.pwm_path.exists() and time.monotonic() < deadline:
            time.sleep(0.01)
        if not self.pwm_path.exists():
            raise PwmError(
                f"PWM channel did not appear after exporting: {self.pwm_path}"
            )

    def open(self, initial_pulse_ns: int) -> None:
        if not 0 < initial_pulse_ns < self.period_ns:
            raise ValueError("initial pulse must be between 0 and the PWM period")
        if not self.chip_path.exists():
            raise PwmError(f"PWM chip does not exist: {self.chip_path}")

        if not self.pwm_path.exists():
            try:
                (self.chip_path / "export").write_text(
                    f"{self.channel}\n",
                    encoding="ascii",
                )
            except OSError as exc:
                raise PwmError(
                    f"cannot export {self.pwm_path.name}; "
                    "check PWM overlay and permissions: "
                    f"{exc}"
                ) from exc
            self._exported_by_us = True
            self._wait_for_channel()

        try:
            # Newly exported channels can have period=0; disabling them first
            # is rejected by some sunxi PWM drivers with EINVAL.
            if self._is_enabled():
                self._write("enable", 0)
            self._write("period", self.period_ns)
            self._write("duty_cycle", initial_pulse_ns)
            if (self.pwm_path / "polarity").exists():
                self._write("polarity", "normal")
            self._write("enable", 1)
        except Exception:
            self.close()
            raise
        self._opened = True

    def set_pulse_ns(self, pulse_ns: int) -> None:
        if not self._opened:
            raise PwmError("PWM channel is not open")
        if not 0 < pulse_ns < self.period_ns:
            raise ValueError("pulse must be between 0 and the PWM period")
        self._write("duty_cycle", pulse_ns)

    def set_pulse_us(self, pulse_us: int) -> None:
        self.set_pulse_ns(pulse_us * 1000)

    @property
    def is_open(self) -> bool:
        return self._opened

    def stop(self) -> None:
        if self.pwm_path.exists() and self._is_enabled():
            self._write("enable", 0)
        self._opened = False

    def close(self) -> None:
        self.stop()
        if self._exported_by_us and self.pwm_path.exists():
            try:
                (self.chip_path / "unexport").write_text(
                    f"{self.channel}\n",
                    encoding="ascii",
                )
            except OSError:
                # Cleanup must not hide the original exception or Ctrl+C.
                pass
            self._exported_by_us = False

    def __enter__(self) -> "SysfsPwm":
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.close()
